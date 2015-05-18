//
//  mcsfinder.cpp  -  Main class for finding MCSes.
//
// by Mark H. Liffiton <liffiton @ eecs . umich . edu>
//
// Copyright (C) 2009, The Regents of the University of Michigan
// See the LICENSE file for details.
//

#include "mcsfinder.h"
#include <algorithm>

using std::cout;
using std::endl;

retVal MCSfinder::solveRaw(Bag *retDeriv, Bag *exclude, int way) {
	Solver s;
	retVal ret;

	if (retDeriv != NULL)
		s.enableDeriv();

	if (p.get_CNF(s, exclude, way)) {
		bool st = s.solve();
		ret = (st) ? SAT : UNSAT;
		if (verbose) addStats(cumulativeStats, s.stats);
		if (verbose) addStats(cumulativeSATStats, s.stats);
	}
	else {
		ret = UNSAT_EARLY;
	}

	if (retDeriv != NULL)
		s.finalDeriv->getAncestorsSum(*retDeriv);

	return ret;
}

// add clauses to a Solver
bool MCSfinder::addBlockingClauses(Solver& sat) {
	bool st = true;
	foreach(vector<MCSBag>, it, MCSes) {
		vec< Lit > newClause;
		foreach(MCSBag, it2, *it) {
			while (*it2 >= (unsigned)sat.nVars)  sat.newVar();
			newClause.push(Lit(*it2+nvars));
		}
		if (st) {
			if (newClause.size() == 1)
				st = sat.addUnit(newClause[0]);
			else
				st = sat.addClause(newClause);
		}
	}
	return st;
}

// solve a single SAT problem, return TRUE if it's SAT, FALSE otherwise
// do *not* clear the search tree (cancelUntil(0) like Solver::solve() does)
//   so we can solve iteratively
bool MCSfinder::itsolve(Solver& sat) {

	SearchParams	params(0.95, 0.999, 0.02);
	double  nof_conflicts = 100;
	double  nof_learnts   = sat.nConstrs() / 3;

	double grow_conflicts = 1.5;
	double grow_learnts = 1.1;

	sat.root_level = sat.decisionLevel;

	lbool status = l_Undef;

	while (status == l_Undef){
		status = sat.search((int)nof_conflicts, (int)nof_learnts, params);
		if (status == l_Undef) {
			nof_conflicts *= grow_conflicts;
			nof_learnts   *= grow_learnts;
		}
	}

	return (status == l_True);
}

// Grow an MSS from a seed satisfiable subset
void MCSfinder::grow(Solver& growsat, Bag& MSS, Bag& MCS, const unsigned int lowbound, const unsigned int highbound) {

	if (verbose)  cout << "Started w/ size: " << MCS.size() << endl;

	growsat.cancelUntil(0);

	// first, inject the seed into the solver
	foreach (Bag, it, MSS) {
		growsat.assume(Lit(*it + nvars));
		growsat.propagate();
	}

	unsigned int curSize = MCS.size();

	// attempt forcing remaining clauses to be satisfied (if still SAT, add clause to growing MSS)
	Bag::iterator it = MCS.begin();
	while (it != MCS.end()) {

		Num i = *it;
		//cerr << "Trying to add " << i+1 << "... " << endl;

		// try adding the current clause
		bool st = growsat.assume(Lit(i + nvars));

		if (st && growsat.propagate() == NULL && itsolve(growsat)) {
			// still satisfiable with current clause forced in

			curSize--;	// because we know we're going to delete the current element,
						// and this might help break the loop checking other clauses earlier

			// Check for any "collateral" satisfied clauses (saves time, avoids calls to itsolve())
			Bag::iterator it2 = it;
			it2++;  // start at the following element
			while (it2 != MCS.end()) {
				Num j = *it2;
				//cerr << "Checking " << j+1 << "... " << endl;

				assert(i!=j);
				//if (i==j) continue;

				it2++;	// so we can erase the current element below if we want to

				if (growsat.model[nvars+j] == 1) {
					// found a "collateral" satisfied clause

					//cerr << "--Removing " << j+1 << "." << endl;
					MCS.erase(j);
					growsat.assume(Lit(j + nvars));
					growsat.propagate();
					--curSize;
					if (lowbound && curSize == lowbound) {
						// done if we've reached the lower bound on MCS size
						if (verbose)  cout << "Lowbound reached." << endl;
						break;
					}
				}
				/* Theoretically, this *can* happen (clause satisfied,
				 * but the y variable still set to false), but it
				 * doesn't happen very often (ever? not sure).
				 * It's probably infrequent enough that it's not
				 * losing anything to leave it out.
				else {

					// get the clause
					vector<Lit> clause;
					p.get_clause(j, clause);

					foreach (vector<Lit>, curLit, clause) {
						// compare it to the model in growsat
						// remember, sign() returns 1 for negated, while the model holds 0 for negated...
						if (growsat.model[var(*curLit)] != sign(*curLit)) {
							cerr << "--Removing " << j+1 << "." << endl;
							MCS.erase(j);
							growsat.assume(Lit(j + nvars));
							growsat.propagate();
							--curSize;
							break;
						}
					}
				}
				// NOTE: left out the lowbound check because it's moved into the conditional above
				 */
			}

			//cerr << "Removing " << i+1 << "." << endl;
			it++; // so we can erase the current element without screwing up the iterator
			MCS.erase(i);
			if (lowbound && curSize == lowbound) {
				// done if we've reached the lower bound on MCS size
				if (verbose)  cout << "Lowbound reached." << endl;
				break;
			}
		}
		else {
			it++; // because we're not using a for loop
			//cerr << "Keeping " << i << "." << endl;
			growsat.propQ.clear();
			growsat.cancel();
		}
	}

	growsat.cancelUntil(0);
	growsat.root_level = growsat.decisionLevel;

	if (verbose)  cout << "Ended w/ size: " << MCS.size() << endl;
}

bool MCSfinder::solve(Solver& sat, unsigned int lowbound, unsigned int highbound) {

	bool foundAny = false;

	SearchParams	params(0.95, 0.999, 0.02);
	double  nof_conflicts = 100;
	double  nof_learnts   = sat.nConstrs() / 3;

	double grow_conflicts = 1.5;
	double grow_learnts = 1.1;

	sat.root_level = sat.decisionLevel;

	lbool status = l_True;

	while (status == l_True) {

		status = l_Undef;

		// main search loop
		while (status == l_Undef){
			status = sat.search((int)nof_conflicts, (int)nof_learnts, params);
			if (status == l_Undef) {
				nof_conflicts *= grow_conflicts;
				nof_learnts   *= grow_learnts;
			}
		}

		numISAT++;

		if (status == l_True) {
			// have a solution

			foundAny = true;

			vec<Lit> newClause;
			MCSBag newMCS; 

			bool doGrow = (lowbound!=highbound);

			// read the MSS/MCS from the current model
			Bag testMSS;
			Bag testMCS;
			for (unsigned int i = 0 ; i < nYvars ; i++) {
				if (sat.model[i+nvars] == false) {
					testMCS.insert(i);
				}
				else if (doGrow) {
					testMSS.insert(i);
				}
			}

			if (doGrow && testMCS.size() != lowbound) {
				grow(sat, testMSS, testMCS, /*p,*/ lowbound, highbound);
			}
			else {
				// no need to call grow() if we already have a smallest possible MCS
				//if (verbose)  cout << "Skipped grow." << endl;
			}

			if (sizeLimit) {
				// exclude any ignored clauses
				Bag dummy;
				set_difference(testMCS.begin(), testMCS.end(), ignored.begin(), ignored.end(), inserter(dummy, dummy.begin()));
				testMCS = dummy;
			}

			if (sizeLimit && sizeLimit < testMCS.size()) {
				// truncate MCS to contain sizeLimit clauses
			
				unsigned int counter = 0; // stores how many clauses we've kept so far

				// first, grab any cannotIgnores
				Bag mustKeep;
				set_intersection(cannotIgnore.begin(),cannotIgnore.end(),testMCS.begin(),testMCS.end(), inserter(mustKeep,mustKeep.begin()));
				foreach(Bag, it, mustKeep) {
					newMCS.push_back(*it);
					testMCS.erase(*it);
					counter++;
				}

				// Just take the first k elements
				foreach(Bag, it, testMCS) {
					if (++counter > sizeLimit) {
						break;
					}
if (verbose)  cout << endl << "Choosing: " << *it << endl;
					newMCS.push_back(*it);
					testMCS.erase(it);
				}

				// update ignored/cannotIgnore
				ignored.insert(testMCS.begin(), testMCS.end());
				cannotIgnore.insert(newMCS.begin(), newMCS.end());
				// force out ignored clauses
				foreach(Bag, it, testMCS) {
					if (verbose)  cout << "Forcing out: " << (*it+1) << endl;
					sat.addUnit(~Lit(*it+nvars));
				}
			}
			else {
				// below limit OR no MCS size limit (-z not specified on command line)
				// just keep all of testMCS
				foreach(Bag, it, testMCS) {
					newMCS.push_back(*it);
				}

				// for ignoring/excluding clauses
				if (sizeLimit) {
					cannotIgnore.insert(newMCS.begin(), newMCS.end());
				}
			}

			foreach(MCSBag, it, newMCS) {
				// output MCS, IF we know it's safe (might not be w/ sizeLimit - see removeSubsumed)
				if (sizeLimit == 0)
					cout << (*it+1) << " ";
				// build newClause
				newClause.push(Lit(*it+nvars));
			}
			if (sizeLimit == 0)
				cout << endl;

			if (maxSAT) return true;

			MCSes.push_back(newMCS);

			if (newClause.size() == 1)
				status = sat.addUnit(newClause[0]);
			else
				status = sat.addClause(newClause);
		}
	}

	return foundAny;
}

void MCSfinder::removeSubsumed() {
	// TODO: Make this more efficient - only check the new MCSes, etc...
	//       Might need to put it into solve()
	//foreach(vector<MCSBag>, it, MCSes) {
	for (unsigned i = 0 ; i < MCSes.size() ; i++) {
		for(vector<MCSBag>::iterator it2 = MCSes.begin() ; it2 != MCSes.end() ; ) {
			if (MCSes[i].size() < it2->size()  // also ensures it != it2
				 && includes(it2->begin(),it2->end() , MCSes[i].begin(),MCSes[i].end())) {
				//cerr << "OMG GOT ONE!" << endl;
				it2 = MCSes.erase(it2);
			 }
			else {
				it2++;
			}
		}
	}
}

bool MCSfinder::checkForMore() {
	bool st = true;
	Solver checkContinue;

	// get the CNF
	if (st)  st = p.get_CNF_Y(checkContinue);

	// force out previous results
	if (st)  st = addBlockingClauses(checkContinue);

	// ignore/exclude clauses
	foreach(Bag, it, ignored) {
		checkContinue.addUnit(~Lit(*it + nvars));
	}

	// no need for bound, we want to see if there's *anything* left

	// look for a solution
	if (st)  st = checkContinue.solve();
	numSAT++;
	if (verbose) addStats(cumulativeStats, checkContinue.stats);
	if (verbose) addStats(cumulativeSATStats, checkContinue.stats);

	return st;
}

// Search for MCSes once the instance has been setup and options set
void MCSfinder::findMCSes() {
	bool st;
	unsigned int bound = initialBound;

	Bag included;

	if (useCores) {
		// Multiple cores
		/*
		unsigned int numCores = 0;
		included = getDisjointCores(&numCores);
		if (numCores > bound) bound = numCores;
		*/
		
		// 2-core intersection
		/*
		unsigned int numCores = 1;
		included = getCoreIntersection();
		*/
		
		// Single core
		///*
		unsigned int numCores = 1;
		included = getCore();
		//*/

		if (verbose) {
			cout << "Initial core count: " << numCores << endl;
			cout << "Initial core(s): " << endl;
			for (Bag::iterator it = included.begin() ; it != included.end() ; it++) {
				cout << *it << " ";
			}
			cout << endl;
			cout << "Core(s) size: " << included.size() << endl;
		}
	}

	while (true) {
		if (verbose) cout << "bound = " << bound << endl;

		bool foundAny = false;

		// find MCSes
		st = true;
		Solver findMCSes;
		if (useCores) {
			findMCSes.enableDeriv();
		}

		// get the CNF
		if (st)  st = p.get_CNF_Y(findMCSes, (useCores) ? &included : NULL);

		// force out previous results
		if (st)  st = addBlockingClauses(findMCSes);

		// ignore/exclude clauses
		foreach(Bag, it, ignored) {
			findMCSes.addUnit(~Lit(*it + nvars));
		}

		// add bound(s) on y-vars
		vec<Lit> ps;
		for (unsigned int i = 0 ; i < nYvars ; i++) {
			// If we're not using cores (standard AtMost
			// w/ all yvars) OR if we are and this is in
			// the set we're supposed to include:
			if (!useCores
					||
					included.find(i+1) != included.end())
				ps.push(~Lit(nvars+i));
		}
		if (st)  st = findMCSes.addAtMost(ps, bound);

		// look for some MCSes
		if (st)  foundAny = solve(findMCSes, bound-boundinc+1, bound);

		if (verbose) printStatsSet(findMCSes.stats);
		if (verbose) addStats(cumulativeStats, findMCSes.stats);
		if (verbose) addStats(cumulativeISATStats, findMCSes.stats);

		if (foundAny && maxSAT) break;

		// check for (and remove) subsumed MCSes (only needed if we're using sizeLimit)
		if (sizeLimit > 0) {
			if (verbose)  cout << "removing subsumed M/PCSes (" << MCSes.size() << " total)" << endl;
			removeSubsumed();
		}

		// Check to see if we need to continue
		// only need to check when previous run found some result
		if (foundAny) {

			// exit if we've reached the threshold of MCS reporting
			if (reportThreshold > 0 && bound >= reportThreshold) {
				break;
			}
		
			// search for existence of more MCSes, given what we've found so far
			// stop if no more solutions exist
			if (verbose) cout << "checking for bound > " << bound << endl;
			if (!checkForMore()) break;
		}

		// Update included with the derivation of this infeasibility
		if (useCores) {
			findMCSes.finalDeriv->getAncestorsSum(included);  // adds to included
			if (verbose) {
				cout << "findMCSes US: " << endl;
				for (Bag::iterator it = included.begin() ; it != included.end() ; it++) {
					cout << *it << " ";
				}
				cout << endl;
			}
		}

		// TODO: Look into extracting/retaining still-relevant learned clauses

		bound += boundinc;
	}


	// TODO: Integrate this w/ only checking new MCSes so we can see *something* as it goes...
	if (sizeLimit > 0) {
		foreach(vector<MCSBag>, it, MCSes) {
			foreach(MCSBag, it2, *it) {
				cout << *it2+1 << " ";
			}
			cout << endl;
		}
	}
}


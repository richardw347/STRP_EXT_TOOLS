/*
 *  MUSbuilder.cpp  -  Constructing MUSes from MCSes
 *  (referred to as MUS covers for historical reasons)
 *
 * by Mark H. Liffiton <liffiton @ eecs . umich . edu>
 *
 * Copyright (C) 2009, The Regents of the University of Michigan
 * See the LICENSE file for details.
 *
 */

#include "MUSbuilder.h"
#include <map>
#include <cassert>

/* ///  HEY!  (11-10-04)  IDEA.
///
///  Employ dynamic programming to remember the result of every subproblem
///  Subproblem = all partial MUSes in these remaining covers
///  Could store some kind of tree of results:  Just the clauses chosen at each
///   spot, so getting them is easy, with pointers to the next spots
///  Potential problem:  Gonna be *lots* of stored results, no?
///   Lesse, one number per node...  max n numbers (n = number clauses in all covers)
///   Hm...  if the tree "reconverges" enough, it might work well... ?
///  Worth trying...  some day.  :)
///
///  Possible implementation:  Standard (STL?) tree structure, hash (of remaining
///   subproblem = remaining covers?) pointing to nodes in tree
///   When get a hit in the hash, make link from current node to hashed location
///   for next time the *current* node is hit in the hash...
///
///  Tricky...  maybe not worth it...  but worth trying.
///
*/

#define DEPTHINDENT for (int i=depth;i-->0;) printf("  ");
#define PRINTCOVERS for (list<Cover>::iterator M_itCover = covers.begin() ; M_itCover != covers.end() ; M_itCover++) { \
					  for (Cover::iterator M_itTEMP = M_itCover->begin() ; M_itTEMP != M_itCover->end() ; M_itTEMP++) { \
						printf("%d ",*M_itTEMP); \
					  } \
					  printf("\n"); \
					}

void MUSbuilder::translateClauses(list<Cover>& MUSCovers, ClauseMap& clauseMapping) {
	// translate the clause numbers
	list<Cover> MUSCoversTranslated;
	for (list<Cover>::iterator it = MUSCovers.begin() ; it != MUSCovers.end() ; it++) {
		Cover newCover;
		for (Cover::iterator it2 = it->begin() ; it2 != it->end() ; it2++) {
			newCover.insert(clauseMapping[*it2]);
		}
		MUSCoversTranslated.push_back(newCover);
	}
	MUSCovers = MUSCoversTranslated;
}

void MUSbuilder::generateMappingSorted(const list<Cover>& MUSCovers, ClauseMap& clauseMapping, ClauseMap& clauseMappingRev) {
	// sort the clauses by how many covers they appear in

	// setup a mapping of clause numbers
	map<Num,Num> clauseFrequency;
	for (list<Cover>::const_iterator it = MUSCovers.begin() ; it != MUSCovers.end() ; it++) {
		for (Cover::iterator it2 = it->begin() ; it2 != it->end() ; it2++) {
			clauseFrequency[*it2]++;
		}
	}
	// need to make the reverse map from the clauseFrequency to get the sorting
	// just use the negation of the frequency for easy reverse sorting
	//
	// in short: insert(-it->second...) sorts with more common clauses first
	//           insert(it->second...) sorts with common clauses last
	//
	// sorting with common clauses last seems to be best
	//
	multimap<int,Num> frequencyToClause;
	for (map<Num,Num>::const_iterator it = clauseFrequency.begin() ; it != clauseFrequency.end() ; it++) {
		frequencyToClause.insert(pair<Num,Num>( (it->second) , it->first ));
	}
	// build the mappings themselves
	Num curNum = 0;
	for (multimap<int,Num>::iterator itClause = frequencyToClause.begin() ; itClause != frequencyToClause.end() ; itClause++) {
		clauseMapping[itClause->second] = curNum;
		clauseMappingRev[curNum] = itClause->second;
		curNum++;
	}
}

void MUSbuilder::generateMappingStraight(const list<Cover>& MUSCovers, ClauseMap& clauseMapping, ClauseMap& clauseMappingRev) {
	// setup a mapping of clause numbers
	// first find the clauses in order (in the set) so that our mapping follows the same order
	set<Num> clauses;
	for (list<Cover>::const_iterator it = MUSCovers.begin() ; it != MUSCovers.end() ; it++) {
		for (Cover::iterator it2 = it->begin() ; it2 != it->end() ; it2++) {
			clauses.insert(*it2);
		}
	}
	// build the mappings themselves
	Num curNum = 0;
	for (set<Num>::iterator itClause = clauses.begin() ; itClause != clauses.end() ; itClause++) {
		clauseMapping[*itClause] = curNum;
		clauseMappingRev[curNum] = *itClause;
		curNum++;
	}
}



// construct MUSes from a set of covers and clauses
// the first will be constructed in polynomial time (*if* not breaking symmetries)
bool MUSbuilder::constructMUS(list<Cover> covers, ClauseAssign curAssign) {

	if (verbose) {
		DEPTHINDENT
		printf("constructMUS\n");
	}

	// quickly take care of any singleton covers (important for speed)
	propagateSingletons(covers, curAssign);

	if (doBB) {
		if (curAssign.numPos + mis_quick(covers) >= bbUpper) {
			if (verbose) {
				DEPTHINDENT
				printf("Bound hit\n");
			}
			return true;
		}
	}

	// reasonable defense against duplicate search nodes this check,
	// immediately before any MUS-output, eliminates the possibility of
	// duplicate MUSes
	//   but be careful to not put the singleton-propagation between them (for
	//   example), because that will mess it up, potentially yielding duplicate
	//   MUSes
	if (isVisited(curAssign)) {
		if(verbose) {
			DEPTHINDENT
			printf("<--Been here before!\n");
		}
		return false;
	}

	if (covers.empty()) {
		// nothing left, so this must be an MUS
		outputMUS(curAssign);

		bbUpper = curAssign.numPos;

		if (verbose) {
			DEPTHINDENT
			printf("<--constructMUS found MUS\n");
		}

		return false;
	}

	// choose a clause to split on
	for (unsigned int curClause = 0 ; curClause < curAssign.size() ; curClause++) {

		if (curAssign[curClause] != 0) { continue; }

		// include this clause in the MUS
		curAssign[curClause] = 1;
		curAssign.numPos++;

		if (verbose) {
		 DEPTHINDENT
		 printf("constructMUS using clause %d\n", curClause);
		 // PRINTCOVERS
		}

		// try splitting on all covers containing the chosen clause
		for (list<Cover>::iterator itCover = covers.begin() ; itCover != covers.end() ; itCover++) {

			assert(!itCover->empty());

			if (itCover->find(curClause) != itCover->end()) {
				// make copies of covers and clauses to be modified
				list<Cover> newcovers = covers;
				ClauseAssign newcurAssign = curAssign;

				// perform the modifications
				removeClauseAndCover(newcovers, newcurAssign, curClause, *itCover);

				// now continue on with the altered copies of covers and clauses
				depth++;
				bool skipRemaining = constructMUS(newcovers, newcurAssign);
				depth--;
				if (skipRemaining) { break; }
			}
		}

		// remove the clause from the MUS
		curAssign[curClause] = -1;
		curAssign.numPos--;
		curAssign.numNeg++;

		// continuing on, remove the clause to simplify things
		// this imposes a lexicographic(ish) order
		//  - it is impossible now to pass a clause to constructMUS (in clauses) that
		//    has a lower number than one already included in the MUS under construction
		// it also maintains an invariant that there are no singleton-covers
		if (!removeClause(covers, curClause, curAssign)) {
			if(verbose) {
				DEPTHINDENT
				printf("<--constructMUS removeClause returned false for %d\n", curClause);
			}
			return false;
		}
	
	}

	if(verbose) printf("<--constructMUS exhausted options\n");

	return false;
}


// Given a selected clause and a cover in which it appears:
//  1) Remove all covers in which the given clause appears.
//  2) Remove all other clauses in the given cover from the other covers.
void MUSbuilder::removeClauseAndCover(list<Cover>& covers, ClauseAssign& curAssign, Num clause, const Cover& cover) {
	if(verbose) {
		DEPTHINDENT
		printf("-->removeClauseAndCover   clause=%d cover=", clause);
		for (Cover::iterator itClause = cover.begin() ; itClause != cover.end() ; itClause++) {
			printf("%d ", *itClause);
		}
		printf("\n");
	}

	// remove all covers containing the given clause
	for (list<Cover>::iterator itCover = covers.begin() ; itCover != covers.end() ; ) {
		if (itCover->find(clause) != itCover->end())
			itCover = covers.erase(itCover);
		else
			itCover++;
	}

	// setup the set of clauses to be removed from the problem (everything in
	// the specified MCS (MUS cover) minus the chosen clause itself (we already
	// removed all MCSes (MUS covers) containing that clause, so this might
	// save a bit of time))
	Cover removeClauses = cover;
	removeClauses.erase(clause);

	// remove the clauses from any covers that contain them
	for (list<Cover>::iterator itCover = covers.begin() ; itCover != covers.end() ; ) {
		Cover newCover;

		set_difference(itCover->begin(),itCover->end() , removeClauses.begin(),removeClauses.end() , inserter(newCover,newCover.begin()));
		if (newCover.size() != itCover->size()) {
			assert(!newCover.empty());

			*itCover = newCover;
			// if anything was removed, maintain the invariant that no cover
			// may fully contain another - only need to check to see if the
			// newly smaller cover is contained in anything else
			maintainNoSubsets(covers, *itCover);

		}
		itCover++;
	}

	// repopulate the set of clauses (some clauses may have only been in
	// now-removed covers)
	ClauseAssign remainingClauses;
	remainingClauses.resize(curAssign.size(), 0);
	for (list<Cover>::iterator itCover = covers.begin() ; itCover != covers.end() ; itCover++) {
		assert(!itCover->empty());
		for (Cover::iterator itClause = itCover->begin() ; itClause != itCover->end() ; itClause++) {
			remainingClauses[*itClause] = 1;
		}
	}

	// this will pick up on any removed clauses
	for (int i = curAssign.size() ; i-->0 ; ) {
		if (curAssign[i] == 0 && !remainingClauses[i]) {
			curAssign[i] = -1;
			curAssign.numNeg++;
		}
	}
}

// Propagate any singleton covers.
// Any singletons in the current subproblem (induced by removing clauses)
// are propagated by this function.  If clause C5 becomes a singleton, for example,
// it is automatically included in the growing MUS (curAssign) and removed from
// the remaining subproblem (covers).
inline void MUSbuilder::propagateSingletons(list<Cover>& covers, ClauseAssign& curAssign) {
	for (list<Cover>::iterator itCover = covers.begin() ; itCover != covers.end() ; ) {
		if (itCover->size() == 1) {
			if (verbose) {
				DEPTHINDENT
				printf("Propagation implies %d\n", *(itCover->begin()));
			}
			assert(curAssign[*(itCover->begin())] == 0);
			curAssign[*(itCover->begin())] = 1;
			curAssign.numPos++;
			itCover = covers.erase(itCover);
		}
		else {
			itCover++;
		}
	}
}

// remove a clause from the covers, used after skipping a clause
// this helps performance *immensely*
inline bool MUSbuilder::removeClause(list<Cover>& covers, Num clause, ClauseAssign& assign) {
	if(verbose) {
		DEPTHINDENT
		printf("-->removeClause   clause=%d\n", clause);
	}

	// remove the clause from any covers that contain it
	for (list<Cover>::iterator itCover = covers.begin() ; itCover != covers.end() ; ) {

		// If it was found (and removed):
		if (itCover->erase(clause) > 0) {

			// 1) return false immediately if the cover is now empty (clause
			// was required)
			if (itCover->empty()) return false;

			// 2) maintain the invariant that no cover may fully contain
			// another only need to check to see if the newly smaller cover is
			// contained in anything else
			maintainNoSubsets(covers, *itCover);
		}
		itCover++;
	}

	return true;
}

// Maintain the invariant that no covers fully contain any others by removing
// any that do.  Function takes a single cover as an input and assumes that it
// is the only cover that could be violating the invariant
inline void MUSbuilder::maintainNoSubsets(list<Cover>& covers, const Cover& modCover) {
	for (list<Cover>::iterator itCover = covers.begin() ; itCover != covers.end() ; ) {
		// 11/10/04 --- We could remove if they're equal, too, but it doesn't seem to help
		//if (modCover.size() <= itCover->size() && modCover != *itCover
		if (modCover.size() < itCover->size()
			 && includes(itCover->begin(),itCover->end() , modCover.begin(),modCover.end()))
			itCover = covers.erase(itCover);
		else
			itCover++;
	}
}

// Check to see if we've visited this assignment before using the beenHere hash set
inline bool MUSbuilder::isVisited(const ClauseAssign& curAssign) {
	return !beenHere.insert(curAssign).second;
}

inline void MUSbuilder::outputMUS(ClauseAssign& curAssign) {
	if (reportEachTime) {
		printf("%Ld: ", (long long)(time(NULL)));
	}

	printf(singletonsStr.c_str());

	for (Num i = 0 ; i < curAssign.size() ; i++) {
		if (curAssign[i] == 1) { 
			printf("%d ", mapToOrig(i)); 
		}
	}
	printf("\n");

	// Alternate printing method:
	// sorting is only needed for making the output nice, and even then only if
	// the mapping was created in a different order than that of the original
	// clause numbers
	/*
	list<Num> MUS;
	for (Num i = 0 ; i < curAssign.size() ; i++) {
		if (curAssign[i] == 1) { 
			MUS.push_back(mapToOrig(i)); 
		}
	}
	MUS.sort();
	foreach(list<Num>, it, MUS) {
		printf("%d ", *it);
	}
	printf("\n");
	*/
}

// Heuristic used to estimate (lower bound) the size of the smallest hitting
// set of the remaining MCSes.  Used only in branch-and-bound search.
// MIS = Maximal Independent Set.  The number of independent sets found is a
// lower bound on the number of elements needed to hit all sets.
// NOTE: This destroys the working dataset - hence passing by copy
inline int MUSbuilder::mis_quick(list<Cover> covers) {
	int result = 0;

	// loop until we're out of covers (all removed by dependence on others)
	while (!covers.empty()) {
		int minLength = 1<<30;
		int curLength = 0;
		Cover removeMCS;

		// loop through the current set of covers, keeping track of the shortest
		for (list<Cover>::iterator it = covers.begin() ; it != covers.end() ; it++) {
			curLength = it->size();
			if (curLength < minLength) {
				minLength = curLength;
				// copy it into removeMCS
				removeMCS = *it;
			}
		}

		// found a new MCS 
		result++;

		// remove intersecting covers (including the newly found cover)
		for (list<Cover>::iterator it = covers.begin() ; it != covers.end() ; ) {
			bool removedSomething = false;
			for (Cover::iterator it2 = removeMCS.begin() ; it2 != removeMCS.end() ; it2++) {
				if (it->find(*it2) != it->end()) {
					removedSomething = true;
					it = covers.erase(it);
					break;
				}
			}
			// nothing erased, so need to increment iterator
			if (!removedSomething) it++;
		}
	}

	return result;
}


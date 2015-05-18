//
//  mcsfinder.h  -  Main class for finding MCSes.
//
// by Mark H. Liffiton <liffiton @ eecs . umich . edu>
//
// Copyright (C) 2009, The Regents of the University of Michigan
// See the LICENSE file for details.
//
#ifndef __MCSFINDER_H
#define __MCSFINDER_H

#include "defs.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <algorithm>

using std::cout;
using std::cerr;
using std::endl;

class MCSfinder {
private:
	Parser p;

	vector<MCSBag> MCSes;

	bool verbose;
	bool reportStats;

	unsigned int boundinc;
	unsigned int initialBound;
	unsigned int sizeLimit;
	unsigned int reportThreshold;
	bool useCores;
	bool maxSAT;

	unsigned int nvars;
	unsigned int nclauses;
	unsigned int nYvars;

	// for remembering ignored/eliminated clauses when truncating MCSes to PCSes
	Bag ignored;
	Bag cannotIgnore;

	// add clauses to a Solver
	bool addBlockingClauses(Solver& sat);

	// solve a single SAT problem, return TRUE if it's SAT, FALSE otherwise
	// do *not* clear the search tree (cancelUntil(0) like Solver::solve() does)
	//   so we can solve iteratively
	bool itsolve(Solver& sat);

	// Grow an MSS from a seed satisfiable subset
	void grow(Solver& growsat, Bag& MSS, Bag& MCS, const unsigned int lowbound=0, const unsigned int highbound=0);

	bool solve(Solver& sat, unsigned int lowbound = 0, unsigned int highbound=0);

	// Check to see if there are any more MCSes (i.e., if the instance is SAT even with blocking clauses for all MCSes found so far)
	bool checkForMore();

	void removeSubsumed();

	// solver stats reporting
	SolverStats cumulativeStats;
	// for Luis Quesada's query about conflicts per SAT and per ISAT
	int numSAT;
	int numISAT;
	SolverStats cumulativeSATStats;
	SolverStats cumulativeISATStats;
	static void addStats(SolverStats& to, SolverStats& from) {
		to.starts += from.starts;
		to.conflicts += from.conflicts;
		to.decisions += from.decisions;
		to.propagations += from.propagations;
		to.inspects += from.inspects;
	}

public:
	MCSfinder() :
		verbose(false),
		reportStats(false),
		boundinc(1),
		initialBound(1),
		sizeLimit(0),
		reportThreshold(0),
		useCores(true),
		maxSAT(false),
		nvars(0),
		nclauses(0),
		nYvars(0),
		numSAT(0),
		numISAT(0)
	{}

	// Solve the plain instance, producing a derivation (core) if retDeriv is specified)
	retVal solveRaw(Bag *retDeriv = NULL, Bag *exclude = NULL, int way = 0);

	// Return a single core of the instance
	Bag getCore() {
		Bag ret;
		solveRaw(&ret);
		return ret;
	}
	// Return an intersection of two cores
	Bag getCoreIntersection() {
		Bag core1;
		Bag core2;
		Bag empty;
		solveRaw(&core1, &empty, 0);
		solveRaw(&core2, &empty, 1);

		Bag result;
		std::insert_iterator<Bag> ii(result, result.begin());

		set_intersection(core1.begin(), core1.end(), core2.begin(), core2.end(), ii);

		return result;
	}
	// Return a union of as many disjoint cores as can be found
	Bag getDisjointCores(unsigned int* numCores) {
		Bag tmp;
		Bag ret;
		*numCores = 0;
		while (solveRaw(&tmp, &ret) != SAT) {
			ret.insert(tmp.begin(), tmp.end());
			tmp.clear();
			(*numCores)++;
		}
		return ret;
	}

	// Search for MCSes once the instance has been setup and options set
	void findMCSes();

	void setCNF(const char* file) {
		p.parseCNF(file);
		nvars = p.getCNFvars();
		nclauses = p.getCNFclauses();
		nYvars = p.getYvars();
	}
	void getMCSes(const char* file) {
		MCSes = p.parseMCSes(file);

		// Set the initial bound based on the largest MCS we've seen
		// NOTE that this assumes they were entered in increasing order of size!
		initialBound = MCSes.back().size();

		// Check to see if we have everything already (just in case someone tries to get clever)
		if (!checkForMore()) {
			cout << "All MCSes included in " << file << ", nothing more to find." << endl;
			exit(1);
		}
	}
	void setPartition(const char* file)	{ p.parsePartition(file); }
	void setClauseMap(const char* file)	{ p.parseClauseMap(file); }
	void setYClauses(const char* file)	{ p.parseYClauses(file); }

	void setVerbose(bool b)		{ verbose = b; }
	void setReportStats(bool b)	{ reportStats = b; }
	void setMaxSAT(bool b)		{ maxSAT = b; }
	void setUseCores(bool b)	{ useCores = b; }

	void setBoundInc(int i)			{ boundinc = i; initialBound = i; 
									  if (useCores) {
									    cerr << "Warning: -x requires -u... setting -u flag automatically." << endl;
										setUseCores(false);
									  }
									}
	void setSizeLimit(int i)		{ sizeLimit = i; }
	void setReportThreshold(int i)	{ reportThreshold = i; }

	void printStats() {
		printStatsSet(cumulativeStats);
		cout << "SAT: " << numSAT << endl;
		printStatsSet(cumulativeSATStats);
		cout << "ISAT: " << numISAT << endl;
		printStatsSet(cumulativeISATStats);
	}
	void printStatsSet(SolverStats& stats) {
		cout << " starts       : " << stats.starts << endl;
		cout << " conflicts    : " << stats.conflicts << endl;
		cout << " decisions    : " << stats.decisions << endl;
		cout << " propagations : " << stats.propagations << endl;
		cout << " inspects     : " << stats.inspects << endl;
	}
};

#endif //__MCSFINDER_H


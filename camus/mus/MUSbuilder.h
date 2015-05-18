/*
 *  MUSbuilder.h  -  Class for constructing MUSes from MUS covers
 *  (referred to as MUS covers for historical reasons)
 *
 * by Mark H. Liffiton <liffiton @ eecs . umich . edu>
 *
 * Copyright (C) 2009, The Regents of the University of Michigan
 * See the LICENSE file for details.
 *
 */

#ifndef MUSBUILDER_H
#define MUSBUILDER_H

#include <list>
#include <set>

#include <hash_set>
#include <hash_map>

#include <vector>
#include <algorithm>

#include <limits>
#include <time.h>
#include <stdio.h>
#include <sstream>

using namespace::std;

#define foreach(x,y,z)  for (x::iterator (y) = (z).begin() ; (y) != (z).end() ; (y)++)

typedef unsigned int Num;
typedef std::set<Num>  Cover;

#ifdef USING_STLPORT
typedef hash_map<Num,int> ClauseMap;
#else
typedef __gnu_cxx::hash_map<Num,int> ClauseMap;
#endif

//typedef vector<char>  ClauseAssign;
class ClauseAssign : public vector<char> {
public:
	int numPos, numNeg;
	ClauseAssign() : numPos(0), numNeg(0) {}
};


// A simple hashing function (for building a hash table of ClauseAssign objects)
struct hashAssign {
	inline size_t operator()(const ClauseAssign& s) const {
		size_t h = 0;
		for (ClauseAssign::const_iterator it = s.begin() ; it != s.end() ; it++) {
			// Thanks to Steven Hayman
			//  (who won a hash-function competition in my algorithms class)
			h = 7 * (h + *it);
		}
		return h;
	}
};

// Equality operator used to compare two ClauseAssign objects
struct eqAssign {
  inline bool operator()(const ClauseAssign& s1, const ClauseAssign& s2) const {
	// assumes they're the same length (safe *only* in this application)
	for (unsigned int i = 0 ; i < s1.size() ; i++) {
		if (s1[i] != s2[i]) {
			return false;
		}
	}
	return true;
  }
};

class MUSbuilder {
private:
	unsigned int depth;
	bool verbose;
	bool reportEachTime;

	// Use branch-and-bound (BB) to find the smallest MUS
	bool doBB;
	// Upper bound used in the branch-and-bound search
	int bbUpper;

	// Singleton MCSes are clauses that will be contained in every MUS;
	// therefore, we need not involve them in any computations.  We store
	// them in a vector and create a string *once*, to print at the beginning
	// of every output MUS, containing the clauses indicated by singleton MCSes.
	vector<Num> singletons;
	string singletonsStr;

	// beenHere is a hash set storing the ClauseAssign objects for any nodes
	// we have already visited.  Used to avoid redundant work.
	// Defined by the object type stored (ClauseAssign),
	// hashing function (hashAssign), and equality operator (eqAssign).
#ifdef USING_STLPORT
	hash_set< ClauseAssign , hashAssign , eqAssign > beenHere;
#else
	__gnu_cxx::hash_set< ClauseAssign , hashAssign , eqAssign > beenHere;
#endif

	ClauseMap clauseMappingRev;	// new to original numbers

	inline int mapToOrig(Num clause) {
		return clauseMappingRev[clause];
	}
	inline void outputMUS(ClauseAssign& curAssign);

	// Given a selected clause and a cover in which it appears:
	//  1) Remove all covers in which the given clause appears.
	//  2) Remove all other clauses in the given cover from the other covers.
	void removeClauseAndCover(list<Cover>& covers, ClauseAssign& curAssign, Num clause, const Cover& cover);

	// Remove a clause from the covers, used after skipping a clause.
	// This helps performance *immensely*.
	inline bool removeClause(list<Cover>& covers, Num clause, ClauseAssign& assign);

	// Check to see if we've visited this assignment before using the beenHere hash set
	inline bool isVisited(const ClauseAssign& curAssign);

	// Maintain the invariant that no covers fully contain any others by removing
	// any that do.  Function takes a single cover as an input and assumes that it
	// is the only cover that could be violating the invariant
	inline void maintainNoSubsets(list<Cover>& covers, const Cover& modCover);

	// Propagate any singleton covers.
	// Any singletons in the current subproblem (induced by removing clauses)
	// are propagated by this function.  If clause C5 becomes a singleton, for example,
	// it is automatically included in the growing MUS (curAssign) and removed from
	// the remaining subproblem (covers).
	inline void propagateSingletons(list<Cover>& covers, ClauseAssign& curAssign);

	// Heuristic used to estimate (lower bound) the size of the smallest hitting
	// set of the remaining MCSes.  Used only in branch-and-bound search.
	// MIS = Maximal Independent Set.  The number of independent sets found is a
	// lower bound on the number of elements needed to hit all sets.
	// NOTE: This destroys the working dataset - hence passing by copy
	inline int mis_quick(list<Cover> covers);


public:
	MUSbuilder() :
		depth(0),
		verbose(false),
		reportEachTime(false),
		doBB(false),
		bbUpper(std::numeric_limits<int>::max()),
		singletonsStr("")
	{ }

	void setVerbose(bool b)			{ verbose = b; }
	void setReportEachTime(bool b)	{ reportEachTime = b; }
	void setDoBB(bool b)			{ doBB = b; }

	// Utility function:
	// Generate a mapping of clause names to numbers.  This ends up being a
	//  mapping of numbers to numbers, but it's important to "compress" the
	//  clause numbers into 0-n.  Additionally, the ordering does matter.  The
	//  code contains two possible orderings that produce different runtimes.
	static void generateMappingSorted(const list<Cover>& MUSCovers, ClauseMap& clauseMapping, ClauseMap& clauseMappingRev);
	static void generateMappingStraight(const list<Cover>& MUSCovers, ClauseMap& clauseMapping, ClauseMap& clauseMappingRev);

	// Utility function:
	// Translate clauses based on the mapping generated by generateMapping*().
	static void translateClauses(list<Cover>& MUSCovers, ClauseMap& clauseMapping);

	// Add a clause mapping (from new clause IDs to original clause indexes)
	void addClauseMapping(ClauseMap& newClauseMappingRev) {
		clauseMappingRev = newClauseMappingRev;
	}

	// Pass singletons into this MUSbuilder object
	// Creates the string used to output them (to be reused instead of
	// recreated each time it's needed).
	void addSingletons(vector<Num>& newSingletons) {
		if (newSingletons.empty()) {return;}
		singletons = newSingletons;
		// make a string of it, too -- to save time when outputting
		ostringstream o;
		foreach(vector<Num>, it, singletons) {
			o << *it << " ";
		}
		singletonsStr = o.str();
	}

	// The main, recursive function
	bool constructMUS(list<Cover> covers, ClauseAssign curAssign);

};

#endif

/**************************************************************************************************
MiniSat -- Copyright (c) 2003-2005, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Constraints_h
#define Constraints_h

#include "SolverTypes.h"
#include <iostream>

// Standard set seems faster than hash_set
#include <set>
typedef std::set<unsigned> Bag;
//#include <ext/hash_set>
//typedef __gnu_cxx::hash_set<unsigned> Bag;

#include <vector>
using std::vector;

// Class for recording derivation (ancestors of derived literals/clauses)
class Deriv {

typedef std::vector<Deriv*> AncPtrs;

protected:
	Bag ancestors;
	AncPtrs ancPtrs;
	bool cycleCheckFlag;
	bool visited;

	inline bool haveAncPtrs() const {
		return !ancPtrs.empty();
	}

	// build our Ancestors set now from the pointers and clear the pointers
	inline void buildAncestors() {
		assert(haveAncPtrs());

		for (unsigned i = 0 ; i < ancPtrs.size() ; i++) {

// RES-TREE VISUALIZATION (see restree_vis.pl)
//std::cout << "P:" << this << "|" << ancPtrs[i] << std::endl;

			const Bag tmpAnc = ancPtrs[i]->getAncestors();

			ancestors.insert(tmpAnc.begin(), tmpAnc.end());
		}
		ancPtrs.clear();

	}

public:
	Deriv() : cycleCheckFlag(false), visited(false) { }

	inline bool empty() const {
		return ancestors.empty() && ancPtrs.empty();
	}

	inline const void getAncestorsSum(Bag& anc) {
		if (visited) return;
		visited = true;

		anc.insert(ancestors.begin(), ancestors.end());
		for (unsigned i = 0 ; i < ancPtrs.size() ; i++) {
			ancPtrs[i]->getAncestorsSum(anc);
		}
	}

	inline const Bag getAncestors() {
// RES-TREE VISUALIZATION (see restree_vis.pl)
// This could print multiple times, but it's not really a big deal...
//if (ancestors.size() == 1) {std::cout << "I:" << this << "|" << *(ancestors.begin()) << std::endl;}
		assert(!cycleCheckFlag);
		cycleCheckFlag = true;
		if (haveAncPtrs()) {
			buildAncestors();
		}
		cycleCheckFlag = false;
		return ancestors;
	}

	inline void addAncestor(const unsigned newAncestor) {
		ancestors.insert(newAncestor);
	}

	inline void addDeriv(Deriv* newDeriv) {
		assert(newDeriv != NULL);
		assert(newDeriv != this);
		assert(!newDeriv->empty());

		// Slow way
		//ancestors.insert(newDeriv->ancestors.begin(), newDeriv->ancestors.end());

		// Fast way (?)
		ancPtrs.push_back(newDeriv);

/*
std::cout << "Adding " << newDeriv << " to " << this << std::endl;
for (unsigned i = 0 ; i < ancPtrs.size() ; i++) {
	std::cout << "  post-add-> " << ancPtrs[i] << std::endl;
}
*/
	}
};

//=================================================================================================
// Constraint abstraction:

class Solver;

class Constr {
	Deriv* myDeriv;

public:
	Constr() : myDeriv(NULL) {
	}
	virtual ~Constr() {
	}

	// Used to see if we're doing anything with derivations in this constraint
	inline bool haveDeriv() {
		return myDeriv != NULL;
	}
	inline void setDeriv(Deriv* newDeriv) {
		assert(myDeriv == NULL);
		myDeriv = newDeriv;
	}
	inline Deriv* getDeriv() {
		assert(myDeriv != NULL);
		return myDeriv;
	}

    virtual void remove    (Solver& S, bool just_dealloc = false) = 0;
    virtual bool propagate (Solver& S, Lit p, bool& keep_watch) = 0;    // ('keep_watch' is set to FALSE beftore call to this method)
    virtual bool simplify  (Solver& S) { return false; };
    virtual void undo      (Solver& S, Lit p) { };
    virtual void calcReason(Solver& S, Lit p, vec<Lit>& out_reason) = 0;
};


//=================================================================================================
// Clauses:


class Clause : public Constr {
    unsigned    size_learnt;
    Lit         data[0];

public:

    int  size        (void)      const { return size_learnt >> 1; }
    bool learnt      (void)      const { return size_learnt & 1; }
    Lit  operator [] (int index) const { return data[index]; }

    // Constructor -- creates a new clause and add it to watcher lists. 
    friend bool Clause_new(Solver& S, const vec<Lit>& ps, bool learnt, Clause*& out_clause, const unsigned id, const bool doDeriv, Deriv* learntDeriv = NULL);

    // Learnt clauses only:
    bool    locked  (const Solver& S) const;
    float&  activity(void) const { return *((float*)&data[size()]); }

    // Constraint interface:
    void remove    (Solver& S, bool just_dealloc = false);
    bool propagate (Solver& S, Lit p, bool& keep_watch);
    bool simplify  (Solver& S);
    void calcReason(Solver& S, Lit p, vec<Lit>& out_reason);

};


//=================================================================================================
// AtMost:


class AtMost : public Constr {
    unsigned int     n;
    unsigned int     counter;
    unsigned int     size;
    Lit     lits[0];

public:

    // Constructor -- creates a new AtMost-constraint and add it to watcher lists.
    friend bool AtMost_new(Solver& S, const vec<Lit>& ps, int n, AtMost*& out, const unsigned id, const bool doDeriv);

    // Constraint interface:
    void remove    (Solver& S, bool just_dealloc = false);
    bool propagate (Solver& S, Lit p, bool& keep_watch);
    bool simplify  (Solver& S);
    void undo      (Solver& S, Lit p);
    void calcReason(Solver& S, Lit p, vec<Lit>& out_reason);
};


//=================================================================================================
#endif

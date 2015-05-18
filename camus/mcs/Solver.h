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

#ifndef Solver_h
#define Solver_h

#include "SolverTypes.h"
#include "Constraints.h"
#include "Queue.h"
#include "VarOrder.h"
#include <iostream>
#include <vector>


//=================================================================================================
// Solver -- the main class:


struct SolverStats {
    int64   starts, decisions, propagations, inspects, conflicts;
    int64   clauses, clauses_literals, learnts, learnts_literals;
    SolverStats(void) : starts(0), decisions(0), propagations(0), inspects(0), conflicts(0)
        , clauses(0), clauses_literals(0), learnts(0), learnts_literals(0) { }
};


struct SearchParams {
    double  var_decay, clause_decay, random_var_freq;    // (reasonable values are: 0.95, 0.999, 0.02)    
    SearchParams(double v = 1, double c = 1, double r = 0) : var_decay(v), clause_decay(c), random_var_freq(r) { }
};


class Solver {
// MHL - 7-15-04 - for now...
//protected:
public:
    bool                ok;             // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
	bool                doDeriv;	    // If TRUE, this solver will compute a derivation (unsat core)
    vec<Constr*>        constrs;        // List of problem constraints.
    vec<Clause*>        learnts;        // List of learnt clauses.
    double              cla_inc;        // Amount to bump next clause with.
    double              cla_decay;      // INVERSE decay factor for clause activity: stores 1/decay.

    vec<double>         activity;       // A heuristic measurement of the activity of a variable.
    double              var_inc;        // Amount to bump next variable with.
    double              var_decay;      // INVERSE decay factor for variable activity: stores 1/decay. Use negative value for static variable order.
    VarOrder            order;          // Keeps track of the decision variable order.

    vec<vec<Constr*> >  watches;        // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<vec<Constr*> >  undos;          // 'undos[var]' is a list of constraints that will be called when 'var' becomes unbound.
    Queue<Lit>          propQ;          // Propagation queue.

    //vec<char>           assigns;        // The current assignments (lbool:s stored as char:s).
    vec<lbool>           assigns;        // The current assignments (lbool:s stored as char:s).
    vec<Lit>            trail;          // List of assignments made. 
    vec<int>            trail_lim;      // Separator indices for different decision levels in 'trail'.
    vec<Constr*>        reason;         // 'reason[var]' is the clause that implied the variables current value, or 'NULL' if none.
    vec<int>            level;          // 'level[var]' is the decision level at which assignment was made.
    int                 root_level;     // Level of first proper decision.
    int                 last_simplify;  // Number of top-level assignments at last 'simplifyDB()'.

	std::vector<Deriv*> derivs;         // Holds pointers to Deriv objects
	std::vector<Deriv*>	var_deriv;      // Point to Deriv objects for decisionLevel 0 variable assignments

    // Temporaries (to reduce allocation overhead):
    //
    vec<char>           analyze_seen;

    // Main internal methods:
    //
    bool        assume       (Lit p);
    void        undoOne      (void);
    void        cancel       (void);
    void        cancelUntil  (int level);
    void        record       (const vec<Lit>& clause, Deriv* deriv);

    void        analyze      (Constr* confl, vec<Lit>& out_learnt, int& out_btlevel, Deriv* out_deriv); // (bt = backtrack)
    bool        enqueue      (Lit fact, Constr* from = NULL, Deriv *deriv = NULL);

	Deriv*      finalDeriv;
    void        analyzeFinalDeriv      (Constr* confl);
    void        analyzeFinalDerivLits  (const vec<Lit>& lits);

	// MHL - 7-23-04
	// copied out of enqueue, for use in AtMost::propagate, to make it not have
	// 44 million function calls to this...
	// - the "safe" part is the assumption that the variable is currently unassigned
	inline void enqueue_safe (Lit p, Constr* from) {
		assigns[var(p)] = toLbool(!sign(p));
		level  [var(p)] = decisionLevel;
		reason [var(p)] = from;
		trail.push(p);
		propQ.insert(p);
	}

    Constr*     propagate    (void);
    void        reduceDB     (void);
    Lit         pickBranchLit(const SearchParams& params);
    lbool       search       (int nof_conflicts, int nof_learnts, const SearchParams& params);
    double      progressEstimate(void);

    // Activity:
    //
    void    varBumpActivity(Lit p) {
        if (var_decay < 0) return;     // (negative decay means static variable order -- don't bump)
        if ( (activity[var(p)] += var_inc) > 1e100 ) varRescaleActivity();
        order.update(var(p)); }
    void    varDecayActivity(void) { if (var_decay >= 0) var_inc *= var_decay; }
    void    varRescaleActivity(void);

    void    claBumpActivity(Clause* c) { if ( (c->activity() += cla_inc) > 1e20 ) claRescaleActivity(); }
    void    claDecayActivity(void) { cla_inc *= cla_decay; }
    void    claRescaleActivity(void);

    //int     decisionLevel(void) { return trail_lim.size(); }
    int     decisionLevel;  // changed to a variable - like nVars, I just prefer this

public:
    Solver(void) : ok               (true)
	             , doDeriv      (false)
                 , cla_inc          (1)
                 , cla_decay        (1)
                 , var_inc          (1)
                 , var_decay        (1)
                 , order            (assigns, activity)
                 , last_simplify    (-1)
				 , finalDeriv(new Deriv())
				 , decisionLevel(0)
				 , nVars(0)
                 , progress_estimate(0)
                 , verbosity(0)
				 { }
   ~Solver(void) {
        for (int i = 0; i < learnts.size(); i++) learnts[i]->remove(*this, true);
        for (int i = 0; i < constrs.size(); i++) constrs[i]->remove(*this, true);
		for (unsigned i = 0 ; i < derivs.size() ; i++) delete derivs[i];
		delete finalDeriv;
	}

	void enableDeriv() {
		doDeriv = true;
	}

    // Helpers: (semi-internal)
    //
	//inline lbool   value(Var x) { return toLbool(assigns[x]); }
	inline lbool   value(Var x) { return assigns[x]; }
	//inline lbool   value(Lit p) { return sign(p) ? ~toLbool(assigns[var(p)]) : toLbool(assigns[var(p)]); }
	inline lbool   value(Lit p) { return sign(p) ? -assigns[var(p)] : assigns[var(p)]; }

    int     nAssigns(void) { return trail.size(); }
    int     nConstrs(void) { return constrs.size(); }
    int     nLearnts(void) { return learnts.size(); }

    // Statistics: (read-only member variable)
    //
    SolverStats stats;

    // Problem specification:
    //
    Var     newVar (void);
    //int     nVars  (void)  { return assigns.size(); }
    int     nVars; // changed to a variable - I just prefer it that way...
	//void	addUnit(Lit p) { if (ok) enqueue(p); }
	// MHL - 7-23-04
	bool	addUnit(Lit p, const unsigned id=0) {
		bool ret = ok;
//std::cout << "addUnit: " << (sign(p)?"~":"") << "x" << var(p) << std::endl;
		if (ok) {
			if (doDeriv) {
				Deriv* tmp = new Deriv();
				derivs.push_back(tmp);
				tmp->addAncestor(id);
				ret = enqueue(p, NULL, tmp);
			}
			else {
				ret = enqueue(p);
			}
		}
		if (!ret && doDeriv) {
			finalDeriv->addAncestor(id);
			finalDeriv->addDeriv(var_deriv[var(p)]);
		}
		return ret;
	}

    // -- constraints:
    friend class Clause;
    friend class AtMost;
    friend bool Clause_new(Solver& S, const vec<Lit>& ps, bool learnt, Clause*& out_clause, const unsigned id, const bool doDeriv, const Deriv* learntDeriv);
    friend bool AtMost_new(Solver& S, const vec<Lit>& ps, int  max   , AtMost*& out_constr, unsigned id, const bool doDeriv);

	inline bool	addClause(const vec<Lit>& ps, const unsigned id=0) {
		bool tmpOK = true;
		Clause* c = NULL;
		if (ok) {
			tmpOK = Clause_new(*this, ps, false, c, id, doDeriv);
			if (c != NULL) constrs.push(c);
		}
		if (!tmpOK && doDeriv) {
			analyzeFinalDerivLits(ps);
			finalDeriv->addAncestor(id);
		}
		return tmpOK;
	}
	inline bool	addAtMost(const vec<Lit>& ps, const int n, const unsigned id=0) {
		if (ok) {
			AtMost* c = NULL;
			ok = AtMost_new(*this, ps, n, c, id, doDeriv);
			if (c != NULL) constrs.push(c);
		}
		return ok;
	}

    // Solving:
    //
    bool    okay(void) { return ok; }
    void    simplifyDB(void);
	bool	solve();
    //bool    solve(const vec<Lit>& assumps);
    //bool    solve(void) { vec<Lit> tmp; return solve(tmp); }

    double      progress_estimate;  // Set by 'search()'.
    //vec<lbool>  model;              // If problem is solved, this vector contains the model (if any).
    vec<bool>  model;              // If problem is solved, this vector contains the model (if any).
    int         verbosity;          // Verbosity level. 0=silent, 1=some progress report, 2=everything
};


//=================================================================================================
#endif

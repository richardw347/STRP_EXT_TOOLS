//
//  parser.h  -  Helper class for parsing various input files
//               and filling Solver objects.
//
// by Mark H. Liffiton <liffiton @ eecs . umich . edu>
//
// Copyright (C) 2009, The Regents of the University of Michigan
// See the LICENSE file for details.
//
#ifndef __PARSER_H
#define __PARSER_H

#include "Solver.h"
#include "defs.h"
#include <iostream>
#include <vector>
#include <set>


class Parser {
public:
	Parser() {
		CNFvars = CNFclauses = Yvars = 0;
		haveGroupMap = false;
	}

	void parseCNF(const char* source);
	vector<MCSBag> parseMCSes(const char* source);
	void parsePartition(const char* source);
	void parseClauseMap(const char* source);
	void parseYClauses(const char* source);
	bool get_CNF_Y(Solver& sat, Bag* anc = NULL);
	bool get_CNF(Solver& sat, Bag* exclude = NULL, int way = 0);

	void get_clause(unsigned int i, vector<Lit>& clause);

	unsigned int getCNFvars() {return CNFvars;}
	unsigned int getCNFclauses() {return CNFclauses;}
	unsigned int getYvars() {return Yvars;}

	inline void getVars(unsigned int clauseNum, std::set<int>& vars) {
		if (!haveGroupMap) {
			for (vector<Lit>::iterator it = origCNF[clauseNum].begin() ; it != origCNF[clauseNum].end() ; it++) {
				vars.insert(var(*it));
			}
			return;
		}

		// if we have a mapping, we need to add *all* vars from the group
		assert(0);  //fix this to use groupmap
	}
	inline bool hasGroupMap() {
		return haveGroupMap;
	}

private:
	unsigned int CNFvars;
	unsigned int CNFclauses;
	unsigned int Yvars;
	bool haveGroupMap;
	std::vector<unsigned int> clauseGroupMap;
	std::vector< vector<Lit> > origCNF;
	std::vector< vector<int> > origYClauses;

};

#endif //__PARSER_H

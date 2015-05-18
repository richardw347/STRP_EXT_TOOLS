//
//  parser.cpp -  Helper class for parsing various input files
//                and filling Solver objects.
//
// by Mark H. Liffiton <liffiton @ eecs . umich . edu>
//
// Copyright (C) 2009, The Regents of the University of Michigan
// See the LICENSE file for details.
//

#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

// Read the problem into origCNF
void Parser::parseCNF(const char* source) {
	ifstream cnfIn(source);
	if (!cnfIn.good()) {cerr << "ERROR: Could not open file " << source << endl; exit(1);}

	string line;
	while (getline(cnfIn, line)) {

		if (line[0] == 'c') {
			// skip comments
			continue;
		}

		istringstream istr(line);

		istr >> std::ws;  // skip whitespace (so eof is triggered if line is entirely whitespace)
		if (istr.eof())  continue;  // blank line

		if (line[0] == 'p') {
			// read the size of the CNF
			// skip "p cnf "
			istr.ignore(16,' ');
			istr.ignore(16,' ');
			// get values
			istr >> CNFvars;
			if (istr.fail()) {cerr << "ERROR: Invalid format line: " << line << endl; exit(1);}
			istr >> CNFclauses;
			if (istr.fail()) {cerr << "ERROR: Invalid format line: " << line << endl; exit(1);}
			continue;
		}

		// now assume it's a clause
		int parsedLit;
		vector<Lit> newClause;

		while (!istr.eof()) {
			istr >> parsedLit;
			if (istr.fail()) {cerr << "ERROR: Invalid clause: " << line << endl; exit(1);}
			if (parsedLit == 0) break;
			if ((unsigned)abs(parsedLit) > CNFvars) {cerr << "ERROR: Invalid variable " << parsedLit << " in line " << line << std::endl; exit(1);}
			int var = abs(parsedLit) - 1;
			newClause.push_back( (parsedLit > 0) ? Lit(var) : ~Lit(var) );

			istr >> std::ws; // skip whitespace (so eof is triggered if line ends with whitespace)
		}
		origCNF.push_back(newClause);
	}
	cnfIn.close();

	if (Yvars == 0) {
		// haven't read any partition file yet, so update nYvars with one per clause
		Yvars = CNFclauses;
	}
}

// Read pre-existing MCSes and return them
vector<MCSBag> Parser::parseMCSes(const char* source) {
	ifstream mcsIn(source);
	if (!mcsIn.good()) {cerr << "ERROR: Could not open file " << source << endl; exit(1);}

	vector<MCSBag> ret;
	unsigned size = 0;
	string line;

	while (getline(mcsIn, line)) {
		MCSBag MCS;
		unsigned int clauseNum;

		istringstream istr(line);

		while (!istr.eof()) {
			istr >> clauseNum;
			if (istr.fail()) {cerr << "ERROR: Invalid group file line: " << line << endl; exit(1);}
			if (clauseNum == 0) {cerr << "ERROR: Invalid clause index " << clauseNum << " in line: " << line << std::endl; exit(1);}

			// Note -1
			MCS.push_back(clauseNum - 1);

			istr >> std::ws; // skip whitespace (so eof is triggered if line ends with whitespace)
		}

		// The last line may be incomplete if we got this from a previous run of camus_mcs;
		// therefore, skip anything that doesn't have enough numbers.  (Will still incorrectly
		// read a line whose last value is truncated but that still has enough values overall.)
		if (MCS.size() < size) break;

		size = MCS.size();
		ret.push_back(MCS);
	}

	return ret;
}

// Read a partition of the clauses to setup groups of clauses for Y variables.
// Partition should be a list of numbers, one per line, each indicating the last
//   clause in each partition, (starting counting from 1), including the last
//   clause of the entire formula
void Parser::parsePartition(const char* source) {
	ifstream partIn(source);
	if (!partIn.good()) {cerr << "ERROR: Could not open file " << source << endl; exit(1);}

	unsigned int current = 1;

	unsigned int split;

	while (!partIn.eof()) {
		partIn >> split;
		if (partIn.fail() && !partIn.eof()) {cerr << "ERROR: Invalid partition in file: " << source << endl; exit(1);}

		while (current <= split) {
			clauseGroupMap.resize(current);
			clauseGroupMap[current-1] = Yvars;
			//cout << "Mapping " << current << " to " << Yvars << endl;
			current++;
		}
		Yvars++;
	}

	haveGroupMap = true;
}

// Read a map of the clauses to setup groups of clauses for Y variables.
// In this function, we take lists of numbers, one *list* per line, each list
//   indicating a group by the clauses contained in it.
void Parser::parseClauseMap(const char* source) {
	ifstream groupIn(source);
	if (!groupIn.good()) {cerr << "ERROR: Could not open file " << source << endl; exit(1);}

	string line;
	while (getline(groupIn, line)) {
		unsigned int clauseNum;

		istringstream istr(line);

		while (!istr.eof()) {
			istr >> clauseNum;
			if (istr.fail()) {cerr << "ERROR: Invalid group file line: " << line << endl; exit(1);}
			if (clauseNum == 0) {cerr << "ERROR: Invalid clause index " << clauseNum << " in line: " << line << std::endl; exit(1);}
			if (clauseNum > clauseGroupMap.size())  clauseGroupMap.resize(clauseNum);
			clauseGroupMap[clauseNum-1] = Yvars;
			//cout << "Mapping " << clauseNum << " to " << Yvars << endl;

			istr >> std::ws; // skip whitespace (so eof is triggered if line ends with whitespace)
		}
		Yvars++;
	}

	haveGroupMap = true;
}


// Read a set of constraints on the y-variables
//  most useful when using groups of y-vars
//  format is like CNF, but without "p cnf..." or comments
//  but it does need 0 at endline (for easier parsing :)
void Parser::parseYClauses(const char* source) {
	ifstream yClauseIn(source);
	if (!yClauseIn.good()) {cerr << "ERROR: Could not open file " << source << endl; exit(1);}

	int lit;
	vector<int> newClause;
	while (!yClauseIn.eof()) {
		yClauseIn >> lit;

		if (yClauseIn.fail()) break;

		if (lit == 0) {
			origYClauses.push_back(newClause);
			newClause.clear();
		}
		else {
			newClause.push_back(lit);
		}
	}
}

// Inserts the problem into a solver from origCNF, adding Y variables.
// Returns FALSE upon immediate conflict
bool Parser::get_CNF_Y(Solver& sat, Bag* instrument_set) {
	while (CNFvars+Yvars > (unsigned int)sat.nVars) sat.newVar();

	for (unsigned i = 0, curY = 0 ; i < origCNF.size() ; i++) {
		vec<Lit> lits;

		// calculate our current Y-var index
		if (haveGroupMap) {
			curY = clauseGroupMap[i];
		}

		// add the given clause to the growing clause
		for (vector<Lit>::iterator it2 = origCNF[i].begin() ; it2 != origCNF[i].end() ; it2++) {
			lits.push(*it2);

			// This creates (clauselit_i -> Y_i) clauses
			// Seems to just slow things down - guess MiniSAT is pretty clever on its own
			// Tested on April 4, 2007 w/ c14.cnf, c15.cnf, c19.cnf, including -x 3,30,etc.
			// All slower w/ helper implications.
			/*
			vec<Lit> helperImplication;
			helperImplication.push(~*it2);
			helperImplication.push(Lit(CNFvars + curY));
			if (!sat.addClause(helperImplication))
				return false;
			*/
		}


		if (instrument_set == NULL || instrument_set->find(curY+1) != instrument_set->end()) {
			// If we're not provided with a set of clauses to instrument, or if
			// the current y-var isn't included in that set, just make the clause and add it.

			// insert y-var
			// putting it at the end of the clause makes things a smidge faster
			//  - My guess: it's because the massive AtMost propagations hit fewer watches
			lits.push(~Lit(CNFvars+curY));

			// don't need to find ancestors with this
			// (by supplying the second parameter of addClause),
			// because it will always be included from now on
			if (!sat.addClause(lits))
				return false;
		}
		else {
			// If we do have a set ot instrument and this y-var is in it:

			// "Tie down" any "loose" variables, those y-vars that are not used anywhere
			// This is surprisingly important to avoid MiniSAT flipping out and making
			// all kinds of decisions on the unused variables.
			sat.addUnit(Lit(CNFvars+curY));
			// Yes, it would be more efficient/elegant/nice/etc. to only create variables
			// we need.  But then I would have to go mess around with all the y-variable-
			// handling code, adding some sort of map between y-variable and clause index, etc.

			if (lits.size() == 1) {
				if (!sat.addUnit(lits[0], curY+1))
					return false;
			}
			else {
				if (!sat.addClause(lits, curY+1))
					return false;
			}
		}

		if (!haveGroupMap)
			curY++;
	}

	// add the y var clauses, if there are any
	for (vector< vector<int> >::iterator it1 = origYClauses.begin() ; it1 != origYClauses.end() ; it1++) {
		vec<Lit> lits;

		for (vector<int>::iterator it2 = it1->begin() ; it2 != it1->end() ; it2++) {
			int var = CNFvars + abs(*it2)-1;
			lits.push( (*it2 > 0) ? Lit(var) : ~Lit(var) );
		}
		if (!sat.addClause(lits))
			return false;
	}

	sat.simplifyDB();
	return sat.okay();
}

// Inserts the problem into a solver from origCNF, without adding Y variables.
// Returns FALSE upon immediate conflict
bool Parser::get_CNF(Solver& sat, Bag* exclude, int way) {
	while (CNFvars > (unsigned int)sat.nVars) sat.newVar();

	unsigned curY = (way) ? origCNF.size()-1 : 0;
	for (int i = (way) ? (int)origCNF.size()-1 : 0 ; (way) ? i >= 0 : (i < (int)origCNF.size()) ; (way) ? i-- : i++ ) {
		vec<Lit> lits;

		// calculate our current Y-var index (for computing derivations)
		if (haveGroupMap) {
			curY = clauseGroupMap[i];
		}

		for (vector<Lit>::iterator it2 = origCNF[i].begin() ; it2 != origCNF[i].end() ; it2++) {
			lits.push(*it2);
		}

		if (exclude == NULL || exclude->find(curY+1) == exclude->end()) {
			if (lits.size() == 1) {
				if (!sat.addUnit(lits[0], curY+1))
					return false;
			}
			else {
				if (!sat.addClause(lits, curY+1))
					return false;
			}
		}

		if (!haveGroupMap)
			(way) ? curY-- : curY++;
	}

	sat.simplifyDB();
	return sat.okay();
}

// Get a single clause from the original CNF (counting from 0)
void Parser::get_clause(unsigned int i, vector<Lit>& clause) {
	clause = origCNF[i];
}


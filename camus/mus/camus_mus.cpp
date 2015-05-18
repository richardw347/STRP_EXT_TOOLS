/*
 *  camus_mus.cpp  -  Construct MUSes from MCSes (aka, find a hypergraph transversal)
 *
 * by Mark H. Liffiton <liffiton @ eecs . umich . edu>
 *
 * Copyright (C) 2009, The Regents of the University of Michigan
 * See the LICENSE file for details.
 *
 */
#define CAMUS_VERSION "1.0.5"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>
#include <getopt.h>
#include <signal.h>
#include <sys/resource.h>
#include <list>
#include <vector>
#include <unistd.h>
#include "MUSbuilder.h"

using namespace::std;

static struct rusage ruse;
#define CPUTIME (getrusage(RUSAGE_SELF,&ruse),\
  ruse.ru_utime.tv_sec + ruse.ru_stime.tv_sec + \
  1e-6 * (ruse.ru_utime.tv_usec + ruse.ru_stime.tv_usec))


// Read the covers into a list of sets
void parseCovers(istream& coversIn, list< Cover >& MUSCovers, vector<Num>& singletons) {
	if (coversIn.fail()) fprintf(stderr, "ERROR! Could not open file.\n"), exit(1);

	string line;
	while (getline(coversIn, line)) {
		istringstream istr(line);
		Cover newCover;
		Num clause;
		while (istr >> clause) {
			newCover.insert(clause);
		}
		if (newCover.size() < 1) {continue;}  // empty line, most likely
		if (newCover.size() == 1) { singletons.push_back(clause); }
		else { MUSCovers.push_back(newCover); }
	}
}

// usage info
static char* prog_name;
static void pusage() {
	cerr
	<< "CAMUS MUS version " << CAMUS_VERSION << endl
	<< "usage: " << prog_name << " [options] [FILE.MCSes]" << endl
	<< endl
	<< "If [FILE.MCSes] is omitted, input is read from STDIN." << endl
	<< endl
	<< "Options:" << endl
	<< "  -v      : verbose" << endl
	<< "  -s      : report stats (runtime to STDERR)" << endl
	<< "  -T      : report a timestamp for every result (for producing anytime graphs)" << endl
	<< "  -b      : use branch-and-bound to find a minimum-cardinality result" << endl
	<< "  -t n    : set an n second timeout" << endl
	<< endl;
	exit(1);
}

// SIGALRM handler
void alarm_handler(int sig) { cerr << "Timeout reached." << endl; exit(1); }



int main(int argc, char** argv) {
	double cpu_time = CPUTIME;
	MUSbuilder builder;
	bool reportTime = false;

	// command line parameters
	prog_name = argv[0];
	while(1) {
		// next option
		int c = getopt(argc, argv, "vsbTt:");
		if (c == -1) break;

		// handle option
		switch (c) {
			case 'v': builder.setVerbose(true); cout << "CAMUS MUS version " << CAMUS_VERSION << endl; break;
			case 's': reportTime = true; break;
			case 'b': builder.setDoBB(true); break;
			case 'T': builder.setReportEachTime(true); break;
			case 't': signal(SIGALRM, alarm_handler); alarm(atoi(optarg)); break;
			default:
				pusage();
		}
	}
	

	list<Cover> MUSCovers;
	vector<Num> singletons;

	// read input
	if (argc - optind < 1) {
		parseCovers(cin, MUSCovers, singletons);
	}
	else if (argc - optind == 1) {
		ifstream coversIn(argv[optind]);
		if (coversIn.fail()) {
			fprintf(stderr, "%s: unable to open file %s\n", prog_name, argv[optind]);
			exit(1);
		}
		parseCovers(coversIn, MUSCovers, singletons);
		coversIn.close();
	}

	builder.addSingletons(singletons);


	ClauseMap clauseMapping;
	ClauseMap clauseMappingRev;

	MUSbuilder::generateMappingSorted(MUSCovers, clauseMapping, clauseMappingRev);
	//MUSbuilder::generateMappingStraight(MUSCovers, clauseMapping, clauseMappingRev);

	builder.addClauseMapping(clauseMappingRev);

	MUSbuilder::translateClauses(MUSCovers, clauseMapping);

	// setup the initial assignment
	ClauseAssign clauseAssignment;
	clauseAssignment.resize(clauseMapping.size(), 0);

	// Main function
	builder.constructMUS(MUSCovers, clauseAssignment);


	if (reportTime) {cerr << setprecision(3) << CPUTIME - cpu_time << endl;}

	return 0;
}


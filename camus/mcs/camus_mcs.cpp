//
//  camus_mcs.cpp  -  Find MCSes for a given CNF formula.
//
// by Mark H. Liffiton <liffiton @ eecs . umich . edu>
//
// Copyright (C) 2009, The Regents of the University of Michigan
// See the LICENSE file for details.
//
//
#define CAMUS_VERSION "1.0.5"

#include "Solver.h"
#include "mcsfinder.h"

#include <time.h>
#include <getopt.h>
#include <iomanip>
#include <signal.h>
#include <sys/resource.h>

using std::cout;
using std::cerr;
using std::endl;
using std::setprecision;

static struct rusage ruse;
#define CPUTIME (getrusage(RUSAGE_SELF,&ruse),\
  ruse.ru_utime.tv_sec + ruse.ru_stime.tv_sec + \
  1e-6 * (ruse.ru_utime.tv_usec + ruse.ru_stime.tv_usec))

// usage info
static char* prog_name;
static void pusage() {
	cerr
	<< "CAMUS MCS version " << CAMUS_VERSION << endl
	<< "usage: " << prog_name << " [options] FILE.cnf [FILE.MCSes]" << endl
	<< endl
	<< "Options:" << endl
	<< "  -v      : verbose" << endl
	<< "  -s      : report stats (runtime to STDERR)" << endl
	<< "  -t n    : set an n second timeout" << endl
	<< "  -j      : just solve the SAT instance (and report result if verbose is on)" << endl
	<< "  -m      : solve Max-SAT by returning first MCS found (incompatible with -z, -x)" << endl
	<< "  -o      : find a single UNSAT core (usually not minimal) using the resolution DAG" << endl
	<< "  -e      : find a single MUS (equivalent to '-z 1')" << endl
	<< "  -x n    : set the bound/increment to n (NOTE: requires -u)" << endl
	<< "  -z n    : truncate each MCS to n clauses" << endl
	<< "  -g FILE : FILE contains groups of clauses (each line is a list of clause numbers (1-based counting) in a group)" << endl
	<< "  -p FILE : FILE contains partitions over clauses (each line contains the last clause (1-based counting) in a partition)" << endl
	<< "  -y FILE : FILE contains clauses defined over the y variables" << endl
	<< "  -l n    : only report MCSes below size n" << endl
	<< "  -u      : disable unsat core extraction/guidance (not recommended: without using cores, CAMUS is much slower)" << endl
	<< endl;
	exit(1);
}

// SIGALRM handler
void alarm_handler(int sig) {
	cerr << "Timeout reached." << endl;
	exit(1);
}

int main(int argc, char** argv) {
	double cpu_time = CPUTIME;

	bool verbose = false;
	bool reportStats = false;
	bool justSolve = false;
	bool oneCore = false;
	
	MCSfinder m;

	// command line parameters
	prog_name = argv[0];
	if (argc < 2) pusage();
	while(1) {
		// next option
		int c = getopt(argc, argv, "vsjumoex:z:l:t:g:p:y:");
		if (c == -1) break;

		// handle option
		switch (c) {
			case 'v': m.setVerbose(true); verbose = true; break;
			case 's': m.setReportStats(true); reportStats = true; break;
			case 'j': justSolve = true; break;
			case 'u': m.setUseCores(false); break;
			case 'm': m.setMaxSAT(true); break;
			case 'o': oneCore = true; break;
			case 'e': m.setSizeLimit(1); break;
			case 'x': m.setBoundInc(atoi(optarg)); break;
			case 'z': m.setSizeLimit(atoi(optarg)); break;
			case 'l': m.setReportThreshold(atoi(optarg)); break;  // experimental
			case 't': signal(SIGALRM, alarm_handler); alarm(atoi(optarg)); break;
			case 'g': m.setClauseMap(optarg); break;  // grouping of clauses
			case 'p': m.setPartition(optarg); break;  // partition of clauses
			case 'y': m.setYClauses(optarg); break;  // extra constraints on y-vars
			default:
				pusage();
		}
	}

	// Read CNF input.
	// Stores the formula in a data structure to fill Solver objects from that.
	m.setCNF(argv[optind]);

	if (optind+1 < argc) {
		// Read pre-existing MCSes
		m.getMCSes(argv[optind+1]);
	}

	// Solve the instance and exit (just to get some data on how long it takes to solve, plain)
	if (justSolve) {
		if (verbose)  cout << "Solving plain formula." << endl;
		retVal ret = m.solveRaw();
		if (verbose) {
			switch (ret) {
				case SAT:
					cout << "Original formula is SAT." << endl;
					break;
				case UNSAT:
					cout << "Original formula is UNSAT." << endl;
					break;
				case UNSAT_EARLY:
					// proven UNSAT without search (propagation only)
					cout << "Original formula is UNSAT_EARLY." << endl;
					break;
			}
		}
		if (verbose)  m.printStats();
		if (reportStats) cerr << setprecision(3) << CPUTIME - cpu_time << endl;

		return 0;
	}

	// Output a single core and exit (mainly for debugging or benchmarking)
	if (oneCore) {
		Bag core = m.getCore();
		for (Bag::iterator it = core.begin() ; it != core.end() ; it++) {
			cout << *it << " ";
		}
		cout << endl;

		if (verbose)  m.printStats();
		if (reportStats) cerr << setprecision(3) << CPUTIME - cpu_time << endl;

		return 0;
	}

	// Find MCSes (w/ options set earlier)
	if (verbose) cout << endl << "Finding MCSes..." << endl;
	m.findMCSes();

	// TODO: report more finely - different stats, etc.
	if (verbose)  m.printStats();
	if (reportStats) cerr << setprecision(3) << CPUTIME - cpu_time << endl;

	return 0;
}


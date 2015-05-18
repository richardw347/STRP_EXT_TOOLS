//
//  defs.h  -  Typedefs and other definitions for camus_mcs.
//
// by Mark H. Liffiton <liffiton @ eecs . umich . edu>
//
// Copyright (C) 2009, The Regents of the University of Michigan
// See the LICENSE file for details.
//
#ifndef __DEFS_H
#define __DEFS_H

#include <set>
#include <vector>

#define foreach(x,y,z)  for (x::iterator (y) = (z).begin() ; (y) != (z).end() ; (y)++)

typedef unsigned int Num;
// NOTE: if Bag is anything other than a set, then some loops may break
//       (they count on erasing not invalidating any iterators of sets)
typedef std::set<Num> Bag;

typedef std::vector<Num> MCSBag;

typedef enum {
	SAT,
	UNSAT,
	UNSAT_EARLY
} retVal;

#endif // __DEFS_H

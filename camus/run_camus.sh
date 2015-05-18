#!/bin/sh
#
# run_camus.sh - Run CAMUS on a CNF file, producing MCSes and MUSes,
#                and optionally recording statistics about the
#                results to a specified file.
#
# This just runs the most basic mode of CAMUS: computing all
# results.  Often, additional options should be specified
# for best performance and to meet the requirements of a
# particular problem, in which case the executables should
# be run directly with the appropriate options.
#
# Author: Mark Liffiton <mliffito@iwu.edu>
#
# Usage:  run_camus.sh TIMEOUT FILE.cnf [RESULTS]
#
# Notes:
#
# The timeout (in seconds) will be used in both phases, so the
# maximum runtime will be TIMEOUT*2.
#
# MCSes/MUSes will be written to FILE.cnf.MCSes and FILE.cnf.MUSes
# If either phase times out, the partial set produced will be written
# to FILE.cnf.MCSes_partial or FILE.cnf.MUSes_partial
#
# If RESULTS is specified, the following data will be appended to
# that file, formatted as a line of CSV:
#
#  filename
#  variables
#  clauses
#  time (sec) to find MCSes
#  return value of camus_mcs (0 indicates success, 1 timeout)
#  MCSes generated (check return value, might not be all)
#  time (sec) to find MUSes
#  return value of camus_mus (0 indicates success, 1 timeout)
#  MUSes generated (check return value, might not be all)
#


#### Settings ########################################
#
# To use additional command line variables, add them
# after the executable names here.  (The timeout is
# already handled below, so don't specify that here.)
#
# E.g.  mcs="/PATH/TO/camus_mcs -z 2"
#
echo "Please set the correct paths in $0 (and delete this message) first."
exit
MCS_EXEC="/PATH/TO/camus_mcs"
MUS_EXEC="/PATH/TO/camus_mus"
#
######################################################


timeout=$1
file=$2
results=$3

## Sanity checks
if [ ! -x "$MCS_EXEC" ] ; then
    echo "\$MCS_EXEC is not an executable file: $MCS_EXEC"
	echo "Please set the path correctly in $0"
	exit 1
fi
if [ ! -x "$MUS_EXEC" ] ; then
    echo "\$MUS_EXEC is not an executable file: $MUS_EXEC"
	echo "Please set the path correctly in $0"
	exit 1
fi
if [ ! -f "$file" ] ; then
    echo "Invalid input file: $file"
	echo "Usage: $0 TIMEOUT FILE.cnf [RESULTS]"
	exit 1
fi


## Helper functions

getCNFSize() {
	echo -n "`grep -P "p\s+cnf\s+\\d+\s+\\d+" $1 | cut -d ' ' -f 3-4 | tr ' ' ','`," >> $results
}


runTest() {
	# bash doesn't seem to do any particularly rigorous scoping, so this ensures no values are carried over
	status="X"
	time="X"

	$1 -t $timeout -s $2 > $3 2> __exestderrout

	status=$?
	time=`cat __exestderrout`

	if [ $status -ne 0 ] ; then
		mv $3 $3_partial
		echo "*"
		cat __exestderrout
	else
		echo -n "."
	fi

	if [ "$results" != "" ] ; then
		echo -n "$time," >> $results
		echo -n "$status," >> $results
		echo -n "`wc -l < $3`," >> $results
	fi

	rm __exestderrout

	return $status
}


## Execution begins here

echo -n $file

if [ "$results" != "" ] ; then
	echo -n "$file," >> $results
	getCNFSize $file
fi

runTest $MCS_EXEC $file $file.MCSes
ret=$?

if [ $ret -eq 0 ] ; then
	runTest $MUS_EXEC $file.MCSes $file.MUSes
else
	if [ "$results" != "" ] ; then
		echo -n ",,," >> $results
	fi
fi

echo
if [ "$results" != "" ] ; then
	echo >> $results
fi


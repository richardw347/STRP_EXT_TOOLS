#!/bin/bash
#
# run_tests.sh -- Run regression tests
#
# Author: Mark Liffiton
# Date: June 2011
#

ALLTESTSFILE="all_tests.txt"

checkfiles() {
	f1=$1
	f2=$2

	# first compare sizes
	size1=`du -b $f1 | cut -f 1`
	size2=`du -b $f2 | cut -f 1`
	if [ $size1 -ne $size2 ] ; then
		echo
		echo "  [31mOutputs differ (size).[0m"
		return 1
	fi

	# then check outputs directly, and if still different then sort and check again
	cmp -s $f1 $f2
	if [ $? -ne 0 ] ; then
		cmp -s <(sort $f1) <(sort $f2)
		if [ $? -ne 0 ] ; then
			echo
			echo "  [31mOutputs differ (contents).[0m"
			return 2
		else
			[ $verbose ] && echo -e "\n  [33mOutputs not equivalent, but sort to same contents.[0m" || echo -ne "\b[33m^.[0m"
		fi
	fi

	return 0
}

viewdiff() {
	f1=$1
	f2=$2
	read -p "  View diff? (T for terminal, V for vimdiff) " -n 1
	echo
	if [[ $REPLY =~ ^[Tt]$ ]]; then
		$DIFF -C3 $f1 $f2
	elif [[ $REPLY =~ ^[Vv]$ ]]; then
		vimdiff $f1 $f2
	fi
}

updateout() {
	f1=$1
	f2=$2
	read -p "  Store new output as correct? " -n 1
	echo
	if [[ $REPLY =~ ^[Yy]$ ]]; then
		echo "  [33mmv $f2 $f1[0m"
		mv $f2 $f1
	fi
}

runtest() {
	cmd=$1
	outfile=$2

	[ $verbose ] && echo -e "\nRunning test: $cmd > $outfile.NEW" || echo -n "."
	$cmd > $outfile.NEW

	checkfiles $outfile $outfile.NEW

	if [ $? -ne 0 ] ; then
		echo
		echo "  [37;41mTest failed:[0m $cmd"
		viewdiff $outfile $outfile.NEW
		updateout $outfile $outfile.NEW
	else
		[ $verbose ] && echo "  [32mTest passed.[0m" || echo -ne "\b[32m*[0m"  # \b = backspace, to overwrite the .
		(( passed++ ))
	fi

	rm -f $outfile.NEW

	(( total++ ))
}

generate() {
	cmd=$1
	outfile=$2
	echo "Generating: $cmd > [32m$outfile[0m"
	$cmd > $outfile
}

##################################################################################

# Make sure we're in the tests directory
cd `dirname $0`

# Test for existence of our test file
if [ ! -e "$ALLTESTSFILE" ] ; then
	echo "ERROR: Tests file $ALLTESTSFILE does not exist."
	exit 1
fi

# Check for colordiff
[ -x "`which colordiff 2>/dev/null`" ] && DIFF="colordiff" || DIFF="diff"

# Set our mode (run as default) and verbosity (0 as default)
verbose=
case "$1" in
	run | "")
		mode="run"
		echo "Running all tests."
		;;
	runverbose)
		mode="run"
		verbose=1
		echo "Running all tests."
		;;
	regenerate)
		read -p "Are you sure you want to regenerate all test outputs (y/n)? "
		if [[ ! $REPLY =~ ^[Yy]$ ]]; then
			echo "Exiting."
			exit 1
		fi
		mode="regenerate" ;;
	nocheck)
		mode="nocheck"
		echo "Running all tests (skipping results checks)"
		;;
	*)
		echo "Invalid mode: $1"
		echo "Options: run, runverbose, regenerate, nocheck"
		exit 1
		;;
esac

total=0
passed=0

# Run a test for each line in our tests file
# Use a separate file descriptor so we still have STDIN for asking questions via read
exec 4< $ALLTESTSFILE  # FD4 attaches to $ALLTESTSFILE
while read LINE <&4; do
	# Split the line into an array
	IFS=, read cmd outfile <<< "$LINE"

	exe=`cut -d " " -f 1 <<< "$cmd"`
	if [ ! -x $exe ] ; then
		echo "ERROR: $exe is not an executable file.  Do you need to run make?"
		exit 1
	fi

	if [ "$mode" = "run" ] ; then
		runtest "$cmd" "$outfile"
	elif [ "$mode" = "regenerate" ] ; then
		generate "$cmd" "$outfile"
	elif [ "$mode" = "nocheck" ] ; then
		$cmd > /dev/null
		echo "."
	fi

done

if [ "$mode" = "run" ] ; then
	echo
	echo "Passed: $passed / $total"
	echo
fi

exit 0


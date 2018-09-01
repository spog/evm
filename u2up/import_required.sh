#!/bin/bash
#
# The "u2up-build" project build script
#
# Copyright (C) 2014-2018 Samo Pogacnik <samo_pogacnik@t-2.net>
# All rights reserved.
#
# This file is part of the "u2up-build" software project.
# This file is provided under the terms of the BSD 3-Clause license,
# available in the LICENSE file of the "u2up-build" software project.
#

set -e
export tab="${tab}-"
pre="${tab}${pro}"
echo "${pre}CALLED: "$0

if [ "x"$1 != "x" ]
then
	comp_u2up_DIR=$1
	echo "${pre}Import using: "$comp_u2up_DIR"/required"
else
	echo "${pre}ERROR: The component specifications path not provided!"
fi

if [ "x"$2 != "x" ]
then
	comp_build_DIR=$2
	echo "${pre}Import into build path: "$comp_build_DIR"/required"
else
	echo "${pre}ERROR: The component build path not provided!"
fi

if [ "x"$3 != "x" ]
then
	comp_repos_DIR=$3
	echo "${pre}Import required from: "$comp_repos_DIR
else
	echo "${pre}ERROR: The common repository path not provided!"
fi

mkdir -p $comp_build_DIR/required
cd $comp_build_DIR/required
while read line; do
	#echo "$line"
	# Remove comments:
	clean_line=`echo $line | sed -e 's|#.*||'`
	#echo "${pre}clean_line: "$clean_line
	if [ "x${clean_line}" != "x" ]
	then
		comp_required_name=`echo $clean_line | sed -e 's| .*||'`
		echo "${pre}Required component name: "$comp_required_name

		comp_required_version=`echo $clean_line | sed -e 's|^.* ||'`
		comp_required_version=`echo $comp_required_version | sed -e 's| .*||'`
		echo "${pre}Required component version: "$comp_required_version

		# Check package:
		cd $comp_repos_DIR
		if [ -f $comp_required_name-$comp_required_version.u2up ]
		then
			sha512sum -c --status $comp_required_name-$comp_required_version.sha
			RET=$?
			if [ $RET -ne 0 ]; then
				echo "${pre}Corrupted package: ${comp_required_name}-${comp_required_version}.u2up!"
				exit 1
			fi
		fi
		cd - > /dev/null
		# Extract package:
		$comp_repos_DIR/$comp_required_name-$comp_required_version.u2up extract
	fi
done < $comp_u2up_DIR/required
exit 0

#!/bin/bash
#
# The "u2up-build" project build rules
#
# Copyright (C) 2017-2018 Samo Pogacnik <samo_pogacnik@t-2.net>
# All rights reserved.
#
# This file is part of the "u2up-build" software project.
# This file is provided under the terms of the BSD 3-Clause license,
# available in the LICENSE file of the "u2up-build" software project.
#
#set -x
set -e

function subpath_set()
{
	_CURDIR_=$(pwd)
	CURDIR=${_CURDIR_##${_SRCDIR_}}
	if [ "x"$CURDIR == "x" ]; then
		SUBPATH="."
	else
		SUBPATH=${CURDIR##\/}
	fi
	echo $SUBPATH
}

function generate_makefile()
{
	export SUBPATH=$1
#	if [ "x"$SUBPATH == "x" ]; then
#		SUBPATH="."
#	fi
#	echo "SUBPATH: "$SUBPATH
#	echo "_SRCDIR_: "$_SRCDIR_
#	echo "_BUILDIR_: "$_BUILDIR_
	TARGET_MK=${_SRCDIR_}/.build/${SUBPATH}/$MAKEFILE
	echo "Generating ${TARGET_MK}"
	source $_SRCDIR_/u2up/version
	echo "comp_version_MAJOR := "$comp_version_MAJOR > ${TARGET_MK}
	echo "comp_version_MINOR := "$comp_version_MINOR >> ${TARGET_MK}
	echo "comp_version_PATCH := "$comp_version_PATCH >> ${TARGET_MK}
	echo "SUBPATH := "$SUBPATH >> ${TARGET_MK}
	echo "PREFIX := "$PREFIX >> ${TARGET_MK}
	echo "export SUBPATH PREFIX" >> ${TARGET_MK}
	echo >> ${TARGET_MK}
	echo "include \${_SRCDIR_}/defaults/default.mk" >> ${TARGET_MK}
#	echo "Done generating ${TARGET_MK}!"
	echo "Done!"
}

function subdirs_config()
{
	export SUBPATH=$1
#	echo "SUBPATH: "$SUBPATH
#	echo "_SRCDIR_: "$_SRCDIR_
#	echo "_BUILDIR_: "$_BUILDIR_

	for SUBDIR in $SUBDIRS;
	do
		export SUBDIR
		mkdir -p ${_SRCDIR_}/.build/${SUBPATH}/${SUBDIR}
		make -C ${_SRCDIR_}/${SUBPATH}/${SUBDIR} -f ${_SRCDIR_}/defaults/configure.mk configure
	done
}

function subdirs_make()
{
	for SUBDIR in $SUBDIRS;
	do
#		echo "SUBPATH: "$SUBPATH
#		echo "SUBDIR: "$SUBDIR
		make -C ${_SRCDIR_}/${SUBPATH}/${SUBDIR} -f ${_SRCDIR_}/.build/${SUBPATH}/${SUBDIR}/${MAKEFILE} $1
	done
}

$@


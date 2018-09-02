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

function subdirs_conf()
{
	export TOPDIR=$1
#	echo "SUBPATH: "$TOPDIR
#	echo "_SRCDIR_: "$_SRCDIR_
#	echo "_BUILDIR_: "$_BUILDIR_

	for SUBDIR in $SUBDIRS;
	do
		export SUBDIR
		mkdir -p $SUBDIR
		make -C $SUBDIR -f $_SRCDIR_/$TOPDIR/$SUBDIR/$CONFFILE conf
	done
}

function generate_makefile()
{
	if [ "x"$TOPDIR == "x" ]; then
		TOPDIR=.
	fi
	if [ "x"$SUBDIR == "x" ]; then
		SUBDIR=.
	fi
	source $_SRCDIR_/u2up/version
	echo "comp_version_MAJOR := "$comp_version_MAJOR > $MAKEFILE
	echo "comp_version_MINOR := "$comp_version_MINOR >> $MAKEFILE
	echo "comp_version_PATCH := "$comp_version_PATCH >> $MAKEFILE
	echo "SUBPATH := "$TOPDIR >> $MAKEFILE
	echo "SUBDIR := "$SUBDIR >> $MAKEFILE
	echo "PREFIX := "$PREFIX >> $MAKEFILE
#	echo "DESTDIR := "$DESTDIR >> $MAKEFILE
	echo "_SRCDIR_ := "$_SRCDIR_ >> $MAKEFILE
#	echo "BUILDIR := "$BUILDIR >> $MAKEFILE
	echo "_BUILDIR_ := "$_BUILDIR_ >> $MAKEFILE
#	echo "export SUBPATH SUBDIR PREFIX DESTDIR _SRCDIR_ _BUILDIR_" >> $MAKEFILE
	echo "export SUBPATH SUBDIR PREFIX _SRCDIR_ _BUILDIR_" >> $MAKEFILE
	echo >> $MAKEFILE
	cat $_SRCDIR_/$TOPDIR/$CONFFILE >> $MAKEFILE
#	echo "Done generating ${TOPDIR}/${MAKEFILE}!"
	echo "Done!"
}

function subdirs_make()
{
	for SUBDIR in $SUBDIRS;
	do
		export SUBDIR
		make -C $SUBDIR -f $MAKEFILE $1
	done
}

$1 $2


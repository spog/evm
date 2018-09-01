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

# U2UP build version:
build_u2up_MAJOR=0
build_u2up_MINOR=1
build_u2up_PATCH=1
build_u2up_version=$build_u2up_MAJOR.$build_u2up_MINOR.$build_u2up_PATCH

export pro="> "
export tab="${tab}-"
pre="${tab}${pro}"

comp_home_dir=""
BUILDIR=""
comp_repo_dir=""
inst_dest_dir=""
comp_clean_build=0
comp_conf_only=0
comp_make_only=0
comp_inst_only=0
comp_pack_only=0

function usage_help ()
{
	echo
	echo "Usage: "$(basename -z $0)" OPTIONS"
	echo
	echo "OPTIONS:"
	echo " [{--repodir=|-r[=]}comp_repo_dir]"
	echo " [{--buildir=|-b[=]}BUILDIR]          Local build path (relative or absolute path)"
	echo " [{--prefix=|-p[=]}PREFIX]            Target installation path absolute prefix"
	echo " [{--destdir=|-d[=]}inst_dest_dir]    Local installation path (DESTDIR is set as absolute path)"
	echo " [--conf] | [--make] | [--inst] |"
	echo " [--compdir=|-c[=]]comp_home_dir      Local component home path (provides SRCDIR via prepare)"
	echo " [{--help|-h}]"
	echo
#	return
	exit 1
}

while [[ $# > 0 ]]
do
#	echo "AAAAAAA:$#"
#	echo "aaaaaaa:$1"
	case $1 in
	-c)
		shift # past argument
		comp_home_dir="${1}"
		;;
	--compdir=*|-c=*)
		comp_home_dir="${1#*=}"
		;;
	-c*)
		comp_home_dir="${1#*c}"
		;;
	-d)
		shift # past argument
		inst_dest_dir="${1}"
		;;
	--destdir=*|-d=*)
		inst_dest_dir="${1#*=}"
		;;
	-d*)
		inst_dest_dir="${1#*d}"
		;;
	-p)
		shift # past argument
		PREFIX="${1}"
		;;
	--prefix=*|-p=*)
		PREFIX="${1#*=}"
		;;
	-p*)
		PREFIX="${1#*p}"
		;;
	-b)
		shift # past argument
		BUILDIR="${1}"
		;;
	--buildir=*|-b=*)
		BUILDIR="${1#*=}"
		;;
	-b*)
		BUILDIR="${1#*b}"
		;;
	-r)
		shift # past argument
		comp_repo_dir="${1}"
		;;
	--repodir=*|-r=*)
		comp_repo_dir="${1#*=}"
		;;
	-r*)
		comp_repo_dir="${1#*r}"
		;;
	--conf)
		shift # past argument
		comp_conf_only=1
		;;
	--make)
		shift # past argument
		comp_make_only=1
		;;
	--inst)
		shift # past argument
		comp_inst_only=1
		;;
	--pack)
		shift # past argument
		comp_pack_only=1
		;;
	--help|-h)
		usage_help
		;;
	*)
		# comp_home_dir or unknown option
		if [ "x"$comp_home_dir == "x" ]
		then
			if [ -d $1 ]
			then
				comp_home_dir=$1
			else
				echo "${pre}ERROR: Unknown option: "$1
				usage_help
			fi
		else
			echo "${pre}ERROR: Unknown option: "$1
			usage_help
		fi
		;;
	esac
	set +e; shift; set -e # to the next token, if any
done
#echo "Home dir: "$comp_home_dir
#echo "Build dir: "$BUILDIR
#echo "Repository dir: "$comp_repo_dir

if [ "x"$comp_home_dir == "x" ]
then
	echo "${pre}ERROR: Setting component home directory is mandatory!"
	usage_help
fi

export pro="> "
export tab="${tab}-"
pre="${tab}${pro}"
echo "${pre}CALLED: "$0
current_work_DIR=$PWD
echo "${pre}from current work dir: "$current_work_DIR
echo "${pre}BUILD.U2UP - version: "$build_u2up_version

# Set absolute U2UP tools directory:
build_u2up_dir=$(dirname $(which $0))
cd $build_u2up_dir
build_u2up_DIR=$PWD
cd - > /dev/null
echo "${pre}U2UP absolute tools dir: "$build_u2up_DIR

# Set absolute COMPONENT HOME directory:
echo "${pre}Using specified component home dir: "$comp_home_dir
cd $comp_home_dir
comp_home_DIR=$PWD
cd - > /dev/null
echo "${pre}component's absolute component home dir: "$comp_home_DIR
comp_home_NAME=$(basename -z $comp_home_DIR)
#echo "${pre}component name: "$comp_home_NAME
comp_u2up_DIR=$comp_home_DIR/u2up
if [ ! -d $comp_u2up_DIR ]
then
	echo "${pre}ERROR: HOME directory of the U2UP component (missing the u2up subdir)!"
	exit 1
fi
echo "${pre}component specifications dir: "$comp_u2up_DIR
if [ ! -f $comp_u2up_DIR/name ]
then
	echo "${pre}ERROR: Missing the u2up/name file!"
	exit 1
fi
echo "${pre}Source u2up/name script:"
. $comp_u2up_DIR/name
echo "${pre}Component requires BUILD.U2UP (minimum compatible) version: "$min_build_u2up_MAJOR.$min_build_u2up_MINOR
echo "${pre}Component NAME: "$comp_name
if [ "x"$min_build_u2up_MAJOR != "x" ]
then
	if [ $min_build_u2up_MAJOR -eq $build_u2up_MAJOR ]
	then
		if [ "x"$min_build_u2up_MINOR != "x" ]
		then
			if [ $min_build_u2up_MINOR -gt $build_u2up_MINOR ]
			then
				echo "${pre}ERROR: Incompatible minimal build.u2up version required (min_build_u2up_MINOR=${min_build_u2up_MINOR})!"
				exit 1
			fi
		else
			echo "${pre}ERROR: Missing minimal build.u2up version required (min_build_u2up_MINOR)!"
			exit 1
		fi
	else
		echo "${pre}ERROR: Incompatible minimal build.u2up version required (min_build_u2up_MAJOR=${min_build_u2up_MAJOR})!"
		exit 1
	fi
else
	echo "${pre}ERROR: Missing minimal build.u2up version required (min_build_u2up_MAJOR)!"
	exit 1
fi

if [ ! -f $comp_u2up_DIR/version ]
then
	echo "${pre}ERROR: Missing the u2up/version file!"
	exit 1
fi
echo "${pre}Source u2up/version script:"
. $comp_u2up_DIR/version
comp_version=$comp_version_MAJOR"."$comp_version_MINOR"."$comp_version_PATCH
echo "${pre}Component VERSION: "$comp_version

conf_u2up_FILE="u2up-conf"
if [ -f $build_u2up_DIR"/"$conf_u2up_FILE ]
then
	. $build_u2up_DIR"/"$conf_u2up_FILE
	echo "${pre}Using configuration file: "$build_u2up_DIR"/"$conf_u2up_FILE
	#cat $build_u2up_DIR"/"$conf_u2up_FILE
else
	echo "${pre}Without "$conf_u2up_FILE" configuration file expected at: "$build_u2up_DIR"/"
fi

if [ ! -f $comp_u2up_DIR/prepare ]
then
	echo "${pre}ERROR: Missing the u2up/prepare file!"
	exit 1
fi
echo "${pre}Source u2up/prepare script:"
SRCDIR=
. $comp_u2up_DIR/prepare
if [ "x"$SRCDIR == "x" ]
then
	echo "${pre}ERROR: Internal source directory not set/provided by the component!"
	exit 1
else
	echo "${pre}Component provided internal source dir: "$SRCDIR
	cd $SRCDIR
	export comp_source_DIR=$PWD
	cd - > /dev/null
fi
export SRCDIR
echo "${pre}absolute source dir: "$comp_source_DIR

# Set absolute BUILD directory:
if [ "x"$BUILDIR == "x" ]
then
	echo "${pre}Using predefined U2UP build location: "$u2up_build_dir
	comp_build_DIR=$u2up_build_dir/$comp_home_NAME
	mkdir -p $comp_build_DIR
	cd $comp_build_DIR
else
	echo "${pre}Using specified U2UP build dir: "$BUILDIR
	mkdir -p $BUILDIR
	cd $BUILDIR
	comp_build_DIR=$PWD
	export comp_build_DIR
fi
export BUILDIR
echo "${pre}absolute build dir: "$comp_build_DIR

if [ "x"$comp_repo_dir == "x" ]
then
	echo "${pre}Using predefined U2UP repository dir: "$u2up_repo_dir
	comp_repo_DIR=$u2up_repo_dir
	mkdir -p $comp_repo_DIR
else
	echo "${pre}Using specified U2UP repository dir: "$comp_repo_dir
	mkdir -p $comp_repo_dir
	cd $comp_repo_dir
	comp_repo_DIR=$PWD
	cd - > /dev/null
fi
echo "${pre}absolute repository dir: "$comp_repo_DIR

if [ ! -f $comp_u2up_DIR/required ]
then
	echo "${pre}ERROR: Missing the u2up/required file!"
	exit 1
fi
$build_u2up_DIR/import_required.sh $comp_u2up_DIR $comp_build_DIR $comp_repo_DIR

if [ ! -f $comp_u2up_DIR/build ]
then
	echo "${pre}ERROR: Missing the u2up/build file!"
	exit 1
fi

if [ "x"$PREFIX == "x" ]
then
	echo "${pre}ERROR: Target installation path absolute PREFIX not set/provided by the component!"
	exit 1
else
	echo "${pre}Target installation path absolute prefix set: "$PREFIX
	if [ "/" != $(echo $PREFIX | sed -e 's%^/.*%/%') ];
	then
		echo "${pre}ERROR: Target installation path PREFIX is not absolute!"
		exit 1
	fi
	mkdir -p $comp_build_DIR$PREFIX
	cd $comp_build_DIR$PREFIX
	prefix_inst_PATH=$PWD
	cd - > /dev/null
	export PREFIX
fi
echo "${pre}absolute build prefix installation dir: "$prefix_inst_PATH

echo "${pre}Source u2up/build script:"
comp_install_dir=
. $comp_u2up_DIR/build
if [ $comp_conf_only -eq 1 ]
then
	component_conf
	exit 0
fi
if [ $comp_make_only -eq 1 ]
then
	component_make
	exit 0
fi

if [ "x"$inst_dest_dir == "x" ]
then
	echo "${pre}Internal installation directory not set/provided by the component!"
	echo "${pre}Local host system installation (may require root privileges)!"
	export DESTDIR="/"
else
	echo "${pre}Component provided internal installation dir: "$inst_dest_dir
	mkdir -p $inst_dest_dir
	cd $inst_dest_dir
	DESTDIR=$PWD
	cd - > /dev/null
	export DESTDIR
fi
echo "${pre}absolute internal installation dir: "$DESTDIR

if [ $comp_inst_only -eq 1 ]
then
	component_inst
	exit 0
fi
if [ $comp_pack_only -eq 0 ]
then
	component_conf
	component_make
	component_inst
fi

if [ ! -f $comp_u2up_DIR/packages ]
then
	echo "${pre}ERROR: Missing the u2up/packages file!"
	exit 1
fi
echo "${pre}Source u2up/packages script:"
. $comp_u2up_DIR/packages

for package_name in "${COMP_PACKAGES[@]}"
do
	echo "${pre}Component's package_name: "$package_name
	if [ "x"$package_name != "x" ]
	then
		subst="COMP_PACKAGE_${package_name}[@]"
		for package_type in "${!subst}"
		do
			echo "${pre}package_type: "$package_type
			if [ "x"$package_type != "x" ]
			then
				case $package_type in
				runtime)
					comp_package_name=${package_name}
					;;
				devel)
					comp_package_name=${package_name}-${package_type}
					;;
				*)
					exit 1
					;;
				esac
#set -x
				subsubst="COMP_PACKAGE_${package_name}_${package_type}[@]"
				echo "${pre}package \""$comp_package_name"\" content: "${!subsubst}
				cd $prefix_inst_PATH
				tar czvf $comp_build_DIR/files.tgz ${!subsubst}
				cd - > /dev/null
				cd $comp_build_DIR
				$build_u2up_DIR/create_package.sh $comp_package_name-$comp_version files.tgz $comp_u2up_DIR/name $comp_u2up_DIR/version $comp_u2up_DIR/required
				if [ "x"$comp_repo_DIR != "x" ]
				then
					echo "${pre}Copy package to the common repository: "$comp_repo_DIR
					cp -pf $comp_package_name-$comp_version* $comp_repo_DIR/
				else
					echo "${pre}The second parameter (a common repository path) not provided (package not copied)!"
				fi
				cd - > /dev/null
				echo "${pre}Package DONE!"
			fi
		done
	fi
done

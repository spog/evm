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

pkg_filename=$1.u2up
sha_filename=$1.sha
echo "${pre}Creating U2UP installation package: "$pkg_filename
cat > $pkg_filename << EOF
#!/bin/bash

EOF
cat >> $pkg_filename << EOF
function name () {
#__NAME_BEGIN__
EOF
cat $3 >> $pkg_filename
cat >> $pkg_filename << EOF
#__NAME_END__
}

function version () {
#__VERSION_BEGIN__
EOF
cat $4 >> $pkg_filename
cat >> $pkg_filename << EOF
#__VERSION_END__
}

if [ "x"\$1 != "x" ]
then
	case \$1 in
	name)
		name;
		echo \$comp_name
		exit 0
		;;
	version)
		version;
		echo \$comp_version_MAJOR.\$comp_version_MINOR.\$comp_version_PATCH
		exit 0
		;;
	required)
		sed '/^__REQUIRED_END__$/,$ d' \$0 | sed '0,/^__REQUIRED_BEGIN__$/d'
		exit 0
		;;
	list)
		echo "Content of the U2UP installation package: "\$0
		echo "Installation path: "\$DESTDIR$PREFIX
		tar_options="tzv"
		;;
	extract)
		echo "Extracting U2UP installation package: "\$0
		echo "Installation path: "\$DESTDIR$PREFIX
		mkdir -p \$DESTDIR$PREFIX
		tar_options="xzv -C \${DESTDIR}${PREFIX}"
		;;
	esac
else
	# default is to view content:
	echo "Content of the U2UP installation package: "\$0
	echo "Installation path: "\$DESTDIR$PREFIX
	tar_options="tzv"
fi

sed '0,/^__FILES_TGZ__$/d' \$0 | tar \$tar_options
exit 0
__REQUIRED_BEGIN__
EOF
cat $5 >> $pkg_filename
cat >> $pkg_filename << EOF
__REQUIRED_END__
__FILES_TGZ__
EOF
cat $2 >> $pkg_filename
chmod 755 $pkg_filename
echo "${pre}Creating U2UP package SHA-512 hash: "$sha_filename
sha512sum -b $pkg_filename > $sha_filename

exit 0

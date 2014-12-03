DESCRIPTION = "The EVM (EVent Machine) software is a C programming framework consisting of the libevm library at its core." 
SECTION = "examples" 
LICENSE = "BSD-3-Clause" 
LIC_FILES_CHKSUM = "file://LICENSE;md5=545019a70ddb0ed2696a201d4724464a"
PV = "1.0.0"

require evm-git.inc
#require evm-local.inc

inherit pkgconfig cmake

do_configure () {
	cmake -DCMAKE_INSTALL_PREFIX=/usr ${S}
}

do_compile() {
	oe_runmake
}
	
do_install() {
	oe_runmake install DESTDIR=${D}
}

FILES_${PN} = "\
/usr/bin/* \ 
/usr/include/* \ 
/usr/lib/* \ 
"


# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)

DESCRIPTION = "PyQt5 display test utility - demonstrates PyQt5 GUI framework on RK3506 DSI display"
SECTION = "graphics"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://pyqt-test.py \
           file://pyqt-simple-test.py \
          "

S = "${WORKDIR}"

DEPENDS = "python3 python3-pyqt5"

RDEPENDS:${PN} = "python3 python3-core python3-pyqt5"

inherit python3-dir

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${S}/pyqt-test.py ${D}${bindir}/pyqt-test
	install -m 0755 ${S}/pyqt-simple-test.py ${D}${bindir}/pyqt-simple-test
}

FILES:${PN} = "${bindir}/pyqt-test ${bindir}/pyqt-simple-test"


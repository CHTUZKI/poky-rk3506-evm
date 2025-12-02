# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)

DESCRIPTION = "ST7701 panel test utility - comprehensive display panel diagnostic tool"
SECTION = "graphics"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://st7701-panel-test.c"

S = "${WORKDIR}"

DEPENDS = "libdrm"

inherit pkgconfig

do_compile() {
	cd ${S}
	${CC} ${CFLAGS} $(pkg-config --cflags libdrm) ${LDFLAGS} st7701-panel-test.c -o st7701-panel-test $(pkg-config --libs libdrm)
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 st7701-panel-test ${D}${bindir}
}


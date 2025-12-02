# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)

DESCRIPTION = "DSI display test utility - cycles through different colors"
SECTION = "graphics"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://dsi-display-test.c"

S = "${WORKDIR}"

DEPENDS = "libdrm"

inherit pkgconfig

do_compile() {
	cd ${S}
	${CC} ${CFLAGS} $(pkg-config --cflags libdrm) ${LDFLAGS} dsi-display-test.c -o dsi-display-test $(pkg-config --libs libdrm) -lpthread
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 dsi-display-test ${D}${bindir}
}


# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)

DESCRIPTION = "LVGL test utility - demonstrates LVGL graphics library functionality"
SECTION = "graphics"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://lvgl-test.c"

S = "${WORKDIR}"

DEPENDS = "lvgl libdrm"

inherit pkgconfig

do_compile() {
	cd ${S}
	${CC} ${CFLAGS} \
		-I${STAGING_INCDIR}/lvgl \
		$(pkg-config --cflags libdrm) \
		${LDFLAGS} \
		lvgl-test.c \
		-o lvgl-test \
		-llvgl \
		$(pkg-config --libs libdrm) \
		-lpthread -lm
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 lvgl-test ${D}${bindir}
}


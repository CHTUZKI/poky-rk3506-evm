# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)

DESCRIPTION = "MP4 video player using GStreamer with software decoder (RK3506G2 has no hardware decoder)"
SECTION = "graphics"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://mp4-player.c \
           file://videoplayback.mp4 \
           file://play-test-video.sh \
           file://check-video-system.sh \
           file://test-video-playback.sh"

S = "${WORKDIR}"

DEPENDS = "gstreamer1.0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-libav"

inherit pkgconfig

do_compile() {
	cd ${S}
	${CC} ${CFLAGS} \
		$(pkg-config --cflags gstreamer-1.0) \
		${LDFLAGS} \
		mp4-player.c \
		-o mp4-player \
		$(pkg-config --libs gstreamer-1.0) \
		-lm
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 mp4-player ${D}${bindir}
	
	# Install test video file
	install -d ${D}${datadir}/mp4-player
	install -m 0644 ${WORKDIR}/videoplayback.mp4 ${D}${datadir}/mp4-player/
	
	# Install test script
	install -m 0755 ${WORKDIR}/play-test-video.sh ${D}${bindir}/
	
	# Install diagnostic script
	install -m 0755 ${WORKDIR}/check-video-system.sh ${D}${bindir}/
	
	# Install complete test script
	install -m 0755 ${WORKDIR}/test-video-playback.sh ${D}${bindir}/
}


# Copyright (C) 2024, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)

require recipes-kernel/linux/linux-yocto.inc
require linux-rockchip.inc

inherit local-git

SRCREV = "${AUTOREV}"
KBRANCH = "develop-6.6"
SRC_URI = " \
	git://github.com/rockchip-linux/kernel.git;protocol=https;branch=${KBRANCH} \
	file://${THISDIR}/files/cgroups.cfg \
	file://${THISDIR}/files/ethernet.cfg \
	file://${THISDIR}/files/drm.cfg \
	file://${THISDIR}/files/backlight.cfg \
	file://${THISDIR}/files/mpp.cfg \
	file://${THISDIR}/files/spi.cfg \
	file://${THISDIR}/files/rk3506g-evb1-v10.dts \
	file://${THISDIR}/files/rk3506g-evb1-v10-dsi.dts \
	file://${THISDIR}/files/rk3506g-evb1-v10-rgb.dts \
	file://${THISDIR}/files/rk3506g-evb1-v10-spi.dts \
"

LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

KERNEL_VERSION_SANITY_SKIP = "1"
LINUX_VERSION ?= "6.6"

SRC_URI:append = " ${@bb.utils.contains('IMAGE_FSTYPES', 'ext4', \
		   'file://${THISDIR}/files/ext4.cfg', \
		   '', \
		   d)}"

# Replace device tree files from meta-rockchip layer
do_configure:append() {
	bbnote "Replacing device tree files from meta-rockchip layer"
	install -d ${S}/arch/arm/boot/dts/rockchip
	
	# Install original device tree (for backward compatibility)
	if [ -f ${THISDIR}/files/rk3506g-evb1-v10.dts ]; then
		bbnote "Installing rk3506g-evb1-v10.dts"
		install -m 0644 ${THISDIR}/files/rk3506g-evb1-v10.dts \
			${S}/arch/arm/boot/dts/rockchip/rk3506g-evb1-v10.dts
	fi
	
	# Install DSI display device tree
	if [ -f ${THISDIR}/files/rk3506g-evb1-v10-dsi.dts ]; then
		bbnote "Installing rk3506g-evb1-v10-dsi.dts"
		install -m 0644 ${THISDIR}/files/rk3506g-evb1-v10-dsi.dts \
			${S}/arch/arm/boot/dts/rockchip/rk3506g-evb1-v10-dsi.dts
	fi
	
	# Install RGB display device tree
	if [ -f ${THISDIR}/files/rk3506g-evb1-v10-rgb.dts ]; then
		bbnote "Installing rk3506g-evb1-v10-rgb.dts"
		install -m 0644 ${THISDIR}/files/rk3506g-evb1-v10-rgb.dts \
			${S}/arch/arm/boot/dts/rockchip/rk3506g-evb1-v10-rgb.dts
	fi
	
	# Install SPI display device tree
	if [ -f ${THISDIR}/files/rk3506g-evb1-v10-spi.dts ]; then
		bbnote "Installing rk3506g-evb1-v10-spi.dts"
		install -m 0644 ${THISDIR}/files/rk3506g-evb1-v10-spi.dts \
			${S}/arch/arm/boot/dts/rockchip/rk3506g-evb1-v10-spi.dts
	fi
}


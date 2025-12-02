# Copyright (C) 2024, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)

# Disable this recipe - using 6.6 instead
COMPATIBLE_MACHINE = "^$"

require recipes-kernel/linux/linux-yocto.inc
require linux-rockchip.inc

inherit local-git

SRCREV = "${AUTOREV}"
KBRANCH = "develop-6.1"
SRC_URI = " \
	git://github.com/rockchip-linux/kernel.git;protocol=https;branch=${KBRANCH} \
	file://${THISDIR}/files/cgroups.cfg \
	file://${THISDIR}/files/ethernet.cfg \
	file://${THISDIR}/files/drm.cfg \
	file://${THISDIR}/files/rk3506g-evb1-v10.dts \
"

LIC_FILES_CHKSUM = "file://COPYING;md5=6bc538ed5bd9a7fc9398086aedcd7e46"

KERNEL_VERSION_SANITY_SKIP = "1"
LINUX_VERSION ?= "6.1"

SRC_URI:append = " ${@bb.utils.contains('IMAGE_FSTYPES', 'ext4', \
		   'file://${THISDIR}/files/ext4.cfg', \
		   '', \
		   d)}"

# Replace device tree file from meta-rockchip layer
do_configure:append() {
	bbnote "Replacing device tree file rk3506g-evb1-v10.dts from meta-rockchip layer"
	install -d ${S}/arch/arm/boot/dts/rockchip
	install -m 0644 ${THISDIR}/files/rk3506g-evb1-v10.dts \
		${S}/arch/arm/boot/dts/rockchip/rk3506g-evb1-v10.dts
}

# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)
#
# Package group for RK3506 GUI development
# This packagegroup can be used to organize all GUI-related packages
#

SUMMARY = "RK3506 GUI development package group"
DESCRIPTION = "Package group containing LVGL and related GUI development tools"

inherit packagegroup

PACKAGES = "${PN}"

RDEPENDS:${PN} = "\
    lvgl \
    lvgl-demo-fb \
    dsi-display-test \
    st7701-panel-test \
    "

# Optional: Add other GUI-related packages here
# RDEPENDS:${PN} += "\
#     qtbase \
#     wayland \
#     "


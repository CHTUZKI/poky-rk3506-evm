# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)
#
# Package group for RK3506 Python GUI development
# This packagegroup includes PyQt5/PyQt6 and related Python GUI packages
#

SUMMARY = "RK3506 Python GUI development package group"
DESCRIPTION = "Package group containing PyQt and related Python GUI development tools for RK3506"

inherit packagegroup

PACKAGES = "${PN}"

# PyQt5 packages (provided by meta-python layer)
RDEPENDS:${PN} = "\
    python3-pyqt5 \
    python3-pyqt5-dev \
    python3-sip \
    python3-sip-dev \
    "

# Optional: Add PyQt6 if needed
# RDEPENDS:${PN} += "\
#     python3-pyqt6 \
#     python3-pyqt6-dev \
#     "

# Optional: Add Qt5 base libraries if needed
# RDEPENDS:${PN} += "\
#     qtbase \
#     qtbase-plugins \
#     "


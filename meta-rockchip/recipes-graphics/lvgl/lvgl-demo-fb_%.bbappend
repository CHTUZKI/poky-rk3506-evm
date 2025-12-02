# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)
#
# Custom LVGL demo configuration for RK3506 EVM
#

# Configure demo to use DRM (recommended for RK3506)
PACKAGECONFIG = "drm"

# Alternative: use framebuffer
# PACKAGECONFIG = "fbdev"

# DRM card device path (default is /dev/dri/card0)
LVGL_CONFIG_DRM_CARD = "/dev/dri/card0"


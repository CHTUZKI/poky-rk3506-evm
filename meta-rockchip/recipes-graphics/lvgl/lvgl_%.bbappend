# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)
#
# Custom LVGL configuration for RK3506 EVM
#

# Configure LVGL to use DRM (default) or framebuffer
# For RK3506, DRM is recommended for better performance
# PACKAGECONFIG = "drm"

# Alternative configurations (uncomment as needed):
# For framebuffer support (more stable, try this if DRM has issues):
PACKAGECONFIG = "fbdev"
# For SDL support (development/testing):
# PACKAGECONFIG = "sdl"

# Customize LVGL memory settings (adjust based on your needs)
# Default is 64KB, increase if you need more complex UIs
LVGL_CONFIG_LV_MEM_SIZE = "(256 * 1024U)"

# Enable logging for debugging (set to 0 for production)
LVGL_CONFIG_LV_USE_LOG = "1"
LVGL_CONFIG_LV_LOG_PRINTF = "1"

# Enable compressed fonts to save space
LVGL_CONFIG_LV_USE_FONT_COMPRESSED = "1"

# Color depth (32-bit is recommended for modern displays)
LVGL_CONFIG_LV_COLOR_DEPTH = "32"

# Use dark theme (set to 1 for dark theme, 0 for light)
LVGL_CONFIG_LV_THEME_DEFAULT_DARK = "0"


# Copyright (c) 2025, Rockchip Electronics Co., Ltd
# Released under the MIT license (see COPYING.MIT for the terms)

# Extend core-image-minimal with SSH, tools, LVGL and display test utilities
CORE_IMAGE_EXTRA_INSTALL += " \
    htop \
    net-tools \
    file \
    lvgl \
    lvgl-demo-fb \
    dsi-display-test \
    st7701-panel-test \
    lvgl-test \
    mp4-player \
    pyqt-test \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav \
    ffmpeg \
    ttf-dejavu-sans \
    ttf-dejavu-sans-mono \
    ttf-dejavu-serif \
    fontconfig \
    fontconfig-utils \
"

# Add SSH server feature (dropbear) and enable root login with empty password
IMAGE_FEATURES += "ssh-server-dropbear debug-tweaks"


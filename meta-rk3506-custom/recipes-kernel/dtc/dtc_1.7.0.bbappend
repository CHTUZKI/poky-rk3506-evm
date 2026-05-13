# Copyright (C) 2024 Rockchip Electronics Co., Ltd.
# Fix for GCC 13+ compatibility - disable const qualifier warnings as errors

# Meson uses c_args for C compiler flags
EXTRA_OEMESON:append = " -Dc_args=-Wno-error=discarded-qualifiers"
EXTRA_OEMESON:append:class-native = " -Dc_args=-Wno-error=discarded-qualifiers"

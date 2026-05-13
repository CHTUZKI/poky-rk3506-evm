# Copyright (C) 2024 Rockchip Electronics Co., Ltd.
# Fix for GCC 14+ compatibility

# Disable -Werror=discarded-qualifiers warning treated as error
CFLAGS:append = " -Wno-error=discarded-qualifiers"
CXXFLAGS:append = " -Wno-error=discarded-qualifiers"

# Also apply to native builds
CFLAGS:append:class-native = " -Wno-error=discarded-qualifiers"
CXXFLAGS:append:class-native = " -Wno-error=discarded-qualifiers"

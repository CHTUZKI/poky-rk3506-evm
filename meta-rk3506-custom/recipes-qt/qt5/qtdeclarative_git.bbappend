# Copyright (C) 2024 Rockchip Electronics Co., Ltd.
# Fix for GCC 13+ compatibility - add missing cstdint include

# Add cstdint to CXXFLAGS to ensure uintptr_t and uint32_t are defined
CXXFLAGS:append = " -include cstdint"
CXXFLAGS:append:class-native = " -include cstdint"

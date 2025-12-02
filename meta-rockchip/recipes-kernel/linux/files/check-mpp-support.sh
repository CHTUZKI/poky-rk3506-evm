#!/bin/sh
# Script to check MPP support in kernel configuration
# This script should be run in the build environment

echo "=== Checking MPP Support in Kernel Configuration ==="
echo ""

KERNEL_DIR="${1:-${WORKDIR}/git}"
if [ ! -d "$KERNEL_DIR" ]; then
    echo "ERROR: Kernel directory not found: $KERNEL_DIR"
    echo "Usage: $0 [kernel_directory]"
    exit 1
fi

CONFIG_FILE="$KERNEL_DIR/.config"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "ERROR: Kernel config file not found: $CONFIG_FILE"
    exit 1
fi

echo "Checking kernel configuration file: $CONFIG_FILE"
echo ""

# Check MPP-related configurations
echo "1. MPP Service:"
grep -E "CONFIG_ROCKCHIP_MPP_SERVICE" "$CONFIG_FILE" || echo "  NOT FOUND"

echo ""
echo "2. VDPU (Video Decoder):"
grep -E "CONFIG_ROCKCHIP_MPP_VDPU|CONFIG_VIDEO_ROCKCHIP_VDEC" "$CONFIG_FILE" || echo "  NOT FOUND"

echo ""
echo "3. VEPU (Video Encoder):"
grep -E "CONFIG_ROCKCHIP_MPP_VEPU" "$CONFIG_FILE" || echo "  NOT FOUND"

echo ""
echo "4. Media Subsystem:"
grep -E "CONFIG_MEDIA_SUPPORT|CONFIG_MEDIA_CONTROLLER" "$CONFIG_FILE" || echo "  NOT FOUND"

echo ""
echo "5. V4L2 Framework:"
grep -E "CONFIG_VIDEO_DEV|CONFIG_V4L2_MEM2MEM_DEV" "$CONFIG_FILE" || echo "  NOT FOUND"

echo ""
echo "6. Memory Management:"
grep -E "CONFIG_DMA_SHARED_BUFFER|CONFIG_ION" "$CONFIG_FILE" || echo "  NOT FOUND"

echo ""
echo "=== Check Complete ==="


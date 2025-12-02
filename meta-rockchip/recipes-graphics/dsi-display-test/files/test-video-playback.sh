#!/bin/sh
# Complete video playback test script for RK3506 EVM
# Run this script to test MP4 video playback

echo "=========================================="
echo "  RK3506 EVM Video Playback Test"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results
PASSED=0
FAILED=0

check_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    PASSED=$((PASSED + 1))
}

check_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAILED=$((FAILED + 1))
}

check_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo "1. Checking system information..."
echo "   User: $(whoami)"
echo "   Groups: $(groups)"
if groups | grep -q video; then
    check_pass "User is in 'video' group"
else
    check_warn "User is NOT in 'video' group (may need for MPP access)"
fi
echo ""

echo "2. Checking DRM devices..."
if [ -c /dev/dri/card0 ]; then
    check_pass "/dev/dri/card0 exists"
    if [ -r /dev/dri/card0 ] && [ -w /dev/dri/card0 ]; then
        check_pass "/dev/dri/card0 is readable and writable"
    else
        check_fail "/dev/dri/card0 permission issue"
        ls -l /dev/dri/card0
    fi
else
    check_fail "/dev/dri/card0 not found"
fi
echo ""

echo "3. Checking MPP devices..."
if [ -c /dev/mpp_service ]; then
    check_pass "/dev/mpp_service exists"
    ls -l /dev/mpp_service
    if [ -r /dev/mpp_service ] && [ -w /dev/mpp_service ]; then
        check_pass "/dev/mpp_service is readable and writable"
    else
        check_fail "/dev/mpp_service permission issue"
    fi
else
    check_warn "/dev/mpp_service not found"
    echo "   Performing detailed MPP diagnostics..."
    echo ""
    echo "   Checking for other MPP devices:"
    ls -l /dev/*mpp* /dev/*vpu* /dev/*rkvdec* 2>/dev/null || echo "   No MPP devices found"
    echo ""
    echo "   Checking for MPP kernel modules:"
    if [ -d /sys/module ]; then
        MODULES=$(ls /sys/module/ | grep -E "mpp|vpu|rkvdec")
        if [ -n "$MODULES" ]; then
            echo "$MODULES" | while read mod; do
                echo "   - $mod (loaded)"
            done
        else
            echo "   No MPP-related modules found in /sys/module"
        fi
    fi
    echo ""
    echo "   Checking dmesg for MPP-related messages:"
    MPP_MSG=$(dmesg | grep -i "mpp\|vpu\|rkvdec")
    if [ -n "$MPP_MSG" ]; then
        echo "$MPP_MSG" | tail -n 5
    else
        echo "   No MPP messages in dmesg"
    fi
    echo ""
    echo "   Checking device tree for MPP nodes:"
    if [ -d /proc/device-tree ]; then
        DTB_NODES=$(ls /proc/device-tree/ | grep -i "mpp\|vpu")
        if [ -n "$DTB_NODES" ]; then
            echo "$DTB_NODES" | while read node; do
                echo "   - $node"
            done
        else
            echo "   No MPP-related nodes found in device tree"
        fi
    else
        echo "   /proc/device-tree not available"
    fi
    echo ""
    echo "   Checking for other video decoding devices:"
    VIDEO_DEVICES=$(ls -l /dev/* 2>/dev/null | grep -E "video|vpu|mpp|rkvdec")
    if [ -n "$VIDEO_DEVICES" ]; then
        echo "$VIDEO_DEVICES"
    else
        echo "   No video decoding devices found"
    fi
fi
echo ""

echo "4. Checking GStreamer plugins..."
if command -v gst-inspect-1.0 >/dev/null 2>&1; then
    check_pass "gst-inspect-1.0 found"
    
    if gst-inspect-1.0 kmssink >/dev/null 2>&1; then
        check_pass "kmssink plugin found"
    else
        check_fail "kmssink plugin not found"
    fi
    
    if gst-inspect-1.0 mppvideodec >/dev/null 2>&1; then
        check_pass "mppvideodec plugin found"
    else
        check_fail "mppvideodec plugin not found"
    fi
    
    if gst-inspect-1.0 qtdemux >/dev/null 2>&1; then
        check_pass "qtdemux plugin found"
    else
        check_fail "qtdemux plugin not found"
    fi
else
    check_fail "gst-inspect-1.0 not found"
fi
echo ""

echo "5. Checking video file..."
VIDEO="/usr/share/mp4-player/videoplayback.mp4"
if [ -f "$VIDEO" ]; then
    check_pass "Video file exists: $VIDEO"
    ls -lh "$VIDEO"
    if command -v file >/dev/null 2>&1; then
        echo "   File type: $(file "$VIDEO" 2>/dev/null)"
    fi
else
    check_fail "Video file not found: $VIDEO"
fi
echo ""

echo "6. Testing simple GStreamer pipeline (test pattern)..."
echo "   Running: gst-launch-1.0 videotestsrc num-buffers=30 ! kmssink"
if timeout 3 gst-launch-1.0 videotestsrc num-buffers=30 ! kmssink >/dev/null 2>&1; then
    check_pass "Test pattern pipeline works"
else
    check_fail "Test pattern pipeline failed"
    echo "   Error output:"
    timeout 3 gst-launch-1.0 videotestsrc num-buffers=30 ! kmssink 2>&1 | head -n 10
fi
echo ""

echo "7. Testing MP4 video playback..."
if [ -f "$VIDEO" ] && [ -x /usr/bin/mp4-player ]; then
    echo "   Running: mp4-player $VIDEO"
    echo "   (This will run for 5 seconds, then stop)"
    echo ""
    timeout 5 mp4-player "$VIDEO" 2>&1 | head -n 30
    PLAYBACK_EXIT=$?
    if [ $PLAYBACK_EXIT -eq 124 ]; then
        check_pass "Video playback started (timeout after 5s is expected)"
    elif [ $PLAYBACK_EXIT -eq 0 ]; then
        check_pass "Video playback completed successfully"
    else
        check_fail "Video playback failed (exit code: $PLAYBACK_EXIT)"
    fi
else
    if [ ! -f "$VIDEO" ]; then
        check_fail "Video file not found"
    fi
    if [ ! -x /usr/bin/mp4-player ]; then
        check_fail "mp4-player not found or not executable"
    fi
fi
echo ""

echo "8. Display subsystem information..."
if [ -d /sys/class/drm ]; then
    echo "   DRM devices:"
    ls /sys/class/drm/ | grep -E "^card" | while read card; do
        echo "   - $card"
    done
    check_pass "DRM subsystem available"
else
    check_fail "DRM subsystem not found"
fi
echo ""

echo "=========================================="
echo "  Test Summary"
echo "=========================================="
echo "Passed: $PASSED"
echo "Failed: $FAILED"
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Please check the output above.${NC}"
    exit 1
fi


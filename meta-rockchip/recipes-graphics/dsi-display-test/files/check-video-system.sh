#!/bin/sh
# Diagnostic script to check video playback system

echo "=== Video Playback System Diagnostics ==="
echo ""

echo "1. Checking DRM devices:"
ls -l /dev/dri/ 2>/dev/null || echo "  ERROR: /dev/dri/ not found"
echo ""

echo "2. Checking DRM card access:"
if [ -c /dev/dri/card0 ]; then
    echo "  /dev/dri/card0 exists"
    if [ -r /dev/dri/card0 ] && [ -w /dev/dri/card0 ]; then
        echo "  /dev/dri/card0 is readable and writable"
    else
        echo "  WARNING: /dev/dri/card0 permissions issue"
        ls -l /dev/dri/card0
    fi
else
    echo "  ERROR: /dev/dri/card0 not found"
fi
echo ""

echo "3. Checking GStreamer installation:"
if command -v gst-inspect-1.0 >/dev/null 2>&1; then
    echo "  gst-inspect-1.0 found"
    echo "  Checking for kmssink:"
    gst-inspect-1.0 kmssink >/dev/null 2>&1 && echo "    kmssink plugin found" || echo "    ERROR: kmssink plugin not found"
    echo "  Checking for mppvideodec:"
    gst-inspect-1.0 mppvideodec >/dev/null 2>&1 && echo "    mppvideodec plugin found" || echo "    ERROR: mppvideodec plugin not found"
    echo "  Checking for qtdemux:"
    gst-inspect-1.0 qtdemux >/dev/null 2>&1 && echo "    qtdemux plugin found" || echo "    ERROR: qtdemux plugin not found"
else
    echo "  ERROR: gst-inspect-1.0 not found"
fi
echo ""

echo "4. Checking video file:"
VIDEO="/usr/share/mp4-player/videoplayback.mp4"
if [ -f "$VIDEO" ]; then
    echo "  $VIDEO exists"
    ls -lh "$VIDEO"
    echo "  File info:"
    file "$VIDEO" 2>/dev/null || echo "    (file command not available)"
else
    echo "  ERROR: $VIDEO not found"
fi
echo ""

echo "5. Testing simple GStreamer pipeline (test pattern):"
echo "  Running: gst-launch-1.0 videotestsrc ! kmssink"
timeout 3 gst-launch-1.0 videotestsrc num-buffers=30 ! kmssink 2>&1 | head -n 20
echo ""

echo "6. Checking MPP (Media Process Platform) devices:"
if [ -c /dev/mpp_service ]; then
    echo "  /dev/mpp_service exists"
    ls -l /dev/mpp_service
    if [ -r /dev/mpp_service ] && [ -w /dev/mpp_service ]; then
        echo "  /dev/mpp_service is readable and writable"
    else
        echo "  WARNING: /dev/mpp_service permissions issue"
    fi
else
    echo "  ERROR: /dev/mpp_service not found"
    echo "  Performing detailed MPP diagnostics..."
    echo ""
    echo "  Checking for other MPP devices:"
    ls -l /dev/*mpp* /dev/*vpu* /dev/*rkvdec* 2>/dev/null || echo "    No MPP devices found"
    echo ""
    echo "  Checking for MPP kernel modules:"
    if [ -d /sys/module ]; then
        MODULES=$(ls /sys/module/ | grep -E "mpp|vpu|rkvdec")
        if [ -n "$MODULES" ]; then
            echo "$MODULES" | while read mod; do
                echo "    - $mod (loaded)"
            done
        else
            echo "    No MPP-related modules found in /sys/module"
        fi
    fi
    echo ""
    echo "  Checking dmesg for MPP-related messages:"
    MPP_MSG=$(dmesg | grep -i "mpp\|vpu\|rkvdec")
    if [ -n "$MPP_MSG" ]; then
        echo "$MPP_MSG" | tail -n 5
    else
        echo "    No MPP messages in dmesg"
    fi
    echo ""
    echo "  Checking device tree for MPP nodes:"
    if [ -d /proc/device-tree ]; then
        DTB_NODES=$(ls /proc/device-tree/ | grep -i "mpp\|vpu")
        if [ -n "$DTB_NODES" ]; then
            echo "$DTB_NODES" | while read node; do
                echo "    - $node"
            done
        else
            echo "    No MPP-related nodes found in device tree"
        fi
    else
        echo "    /proc/device-tree not available"
    fi
    echo ""
    echo "  Checking for other video decoding devices:"
    VIDEO_DEVICES=$(ls -l /dev/* 2>/dev/null | grep -E "video|vpu|mpp|rkvdec")
    if [ -n "$VIDEO_DEVICES" ]; then
        echo "$VIDEO_DEVICES"
    else
        echo "    No video decoding devices found"
    fi
fi
echo ""

echo "7. Checking user groups:"
echo "  Current user: $(whoami)"
echo "  Current groups: $(groups)"
if groups | grep -q video; then
    echo "  User is in 'video' group"
else
    echo "  WARNING: User is NOT in 'video' group"
fi
echo ""

echo "8. Checking display subsystem:"
if [ -d /sys/class/drm ]; then
    echo "  DRM sysfs found:"
    ls /sys/class/drm/
    echo ""
    for card in /sys/class/drm/card*; do
        if [ -d "$card" ]; then
            echo "  Card: $(basename $card)"
            ls "$card/" | grep -E "card|connector|crtc" | head -n 5
        fi
    done
else
    echo "  WARNING: /sys/class/drm not found"
fi
echo ""

echo "=== Diagnostics Complete ==="


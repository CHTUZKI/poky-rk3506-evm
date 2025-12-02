#!/bin/sh
# Simple script to play the test video

VIDEO_FILE="/usr/share/mp4-player/videoplayback.mp4"

if [ ! -f "$VIDEO_FILE" ]; then
    echo "Error: Test video not found at $VIDEO_FILE"
    exit 1
fi

echo "Playing test video: $VIDEO_FILE"
mp4-player "$VIDEO_FILE"


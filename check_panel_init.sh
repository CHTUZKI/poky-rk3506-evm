#!/bin/bash
# 检查面板驱动初始化的脚本

echo "=========================================="
echo "面板驱动初始化检查"
echo "=========================================="
echo ""

echo "1. 检查内核日志中的面板驱动信息："
echo "----------------------------------------"
dmesg | grep -iE "st7701|sitronix|panel.*probe|dsi.*probe|dw_mipi_dsi.*bind" | head -20
echo ""

echo "2. 检查 DSI 相关日志："
echo "----------------------------------------"
dmesg | grep -iE "dw-mipi-dsi|dw_mipi_dsi|atomic_pre_enable|atomic_enable" | head -20
echo ""

echo "3. 检查面板 prepare/enable 调用："
echo "----------------------------------------"
dmesg | grep -iE "\[ST7701\]|drm_panel_prepare|drm_panel_enable|panel.*prepare|panel.*enable" | head -20
echo ""

echo "4. 检查 GPIO 状态（GPIO1_D3 = gpio-59）："
echo "----------------------------------------"
if [ -d /sys/kernel/debug/gpio ]; then
    cat /sys/kernel/debug/gpio | grep -E "gpio-59|GPIO1_D3" || echo "未找到 GPIO1_D3 信息"
else
    echo "/sys/kernel/debug/gpio 不存在，需要挂载 debugfs"
fi
echo ""

echo "5. 检查面板设备节点："
echo "----------------------------------------"
if [ -d /sys/class/drm/card0-DSI-1 ]; then
    echo "DSI 连接器存在："
    ls -la /sys/class/drm/card0-DSI-1/
    echo ""
    echo "状态："
    cat /sys/class/drm/card0-DSI-1/status 2>/dev/null || echo "无法读取状态"
else
    echo "DSI 连接器不存在"
fi
echo ""

echo "6. 检查面板驱动模块："
echo "----------------------------------------"
lsmod | grep -iE "st7701|panel|dsi" || echo "未找到相关模块（可能是内置驱动）"
echo ""

echo "7. 检查设备树中的面板配置："
echo "----------------------------------------"
if [ -d /sys/firmware/devicetree/base ]; then
    find /sys/firmware/devicetree/base -name "*panel*" -o -name "*st7701*" 2>/dev/null | head -10
    echo ""
    echo "检查面板节点的 reset-gpios："
    find /sys/firmware/devicetree/base -path "*/panel@0/reset-gpios" 2>/dev/null | while read f; do
        echo "找到: $f"
        cat "$f" 2>/dev/null | od -An -tx1 | tr -d ' \n' && echo ""
    done
else
    echo "设备树信息不可用"
fi
echo ""

echo "8. 检查完整的启动日志（最近1000行）："
echo "----------------------------------------"
dmesg | tail -1000 | grep -iE "st7701|panel|dsi|display" | head -30
echo ""

echo "=========================================="
echo "检查完成"
echo "=========================================="


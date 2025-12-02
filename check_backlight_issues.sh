#!/bin/bash
# 检查backlight驱动的三个潜在问题

echo "=========================================="
echo "检查 Backlight 驱动问题"
echo "=========================================="
echo ""

# 1. 检查驱动是否被编译进内核（而不是模块）
echo "1. 检查驱动配置..."
echo "----------------------------------------"
if [ -f "meta-rockchip/recipes-kernel/linux/files/backlight.cfg" ]; then
    echo "✓ 找到 backlight.cfg 配置文件"
    echo "  内容："
    cat meta-rockchip/recipes-kernel/linux/files/backlight.cfg | grep -v "^#" | grep -v "^$"
else
    echo "✗ 未找到 backlight.cfg 配置文件"
fi

if grep -q "backlight.cfg" meta-rockchip/recipes-kernel/linux/linux-rockchip_6.6.bb; then
    echo "✓ backlight.cfg 已添加到 SRC_URI"
else
    echo "✗ backlight.cfg 未添加到 SRC_URI"
fi

echo ""
echo "检查 pwm_bl.c 中的驱动注册方式..."
if grep -q "fs_initcall\|module_init" meta-rockchip/recipes-kernel/linux/files/pwm_bl.c; then
    echo "✓ 找到驱动注册代码"
    echo "  注册方式："
    grep -E "fs_initcall|module_init" meta-rockchip/recipes-kernel/linux/files/pwm_bl.c | head -2
    if grep -q "fs_initcall" meta-rockchip/recipes-kernel/linux/files/pwm_bl.c; then
        echo "  → 使用 fs_initcall: 驱动会被编译进内核（built-in）"
    fi
    if grep -q "module_init" meta-rockchip/recipes-kernel/linux/files/pwm_bl.c; then
        echo "  → 使用 module_init: 驱动可能被编译为模块"
    fi
else
    echo "✗ 未找到驱动注册代码"
fi

echo ""
echo "=========================================="
echo "2. 检查设备树配置..."
echo "----------------------------------------"
if grep -q "compatible.*pwm-backlight" meta-rockchip/recipes-kernel/linux/files/rk3506g-evb1-v10.dts; then
    echo "✓ 设备树中有 pwm-backlight compatible 节点"
    echo "  节点内容："
    grep -A 5 "backlight:" meta-rockchip/recipes-kernel/linux/files/rk3506g-evb1-v10.dts | head -6
else
    echo "✗ 设备树中未找到 pwm-backlight compatible 节点"
fi

echo ""
echo "=========================================="
echo "3. 检查驱动注册调试信息..."
echo "----------------------------------------"
if grep -q "\[DEBUG\].*pwm_backlight_driver_init" meta-rockchip/recipes-kernel/linux/files/pwm_bl.c; then
    echo "✓ 驱动注册函数中有调试信息"
    echo "  调试信息位置："
    grep -n "\[DEBUG\].*pwm_backlight_driver_init" meta-rockchip/recipes-kernel/linux/files/pwm_bl.c | head -3
else
    echo "✗ 驱动注册函数中未找到调试信息"
fi

if grep -q "\[DEBUG\].*pwm_backlight_probe" meta-rockchip/recipes-kernel/linux/files/pwm_bl.c; then
    echo "✓ probe函数中有调试信息"
else
    echo "✗ probe函数中未找到调试信息"
fi

echo ""
echo "=========================================="
echo "总结和建议："
echo "=========================================="
echo ""
echo "如果驱动未编译进内核："
echo "  1. 确保 backlight.cfg 包含 CONFIG_BACKLIGHT_PWM=y"
echo "  2. 确保 backlight.cfg 已添加到 SRC_URI"
echo "  3. 重新编译内核"
echo ""
echo "如果驱动注册日志在启动早期被刷掉："
echo "  1. 使用 dmesg -w 实时查看日志"
echo "  2. 查看 /var/log/kern.log 或 /var/log/messages"
echo "  3. 使用 earlyprintk 或 earlycon 查看早期日志"
echo ""
echo "如果设备树节点未被注册为platform设备："
echo "  1. 确认节点有 compatible = \"pwm-backlight\""
echo "  2. 确认节点有 status = \"okay\""
echo "  3. 检查内核是否启用了 OF (Device Tree) 支持"
echo ""


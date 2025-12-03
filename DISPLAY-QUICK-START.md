# RK3506 显示屏配置快速指南

## ✅ 已完成的配置

方案 A（MACHINE 配置方式）已成功实施！

---

## 🚀 立即开始使用

### **选项 1: 使用 DSI 屏幕（480x800）**

```bash
cd /home/xuning/poky-rk3506-evm/poky
source oe-init-build-env

# 编辑 conf/local.conf，添加或修改：
MACHINE = "rockchip-rk3506-evb-dsi"

# 构建
bitbake core-image-minimal
```

### **选项 2: 使用 RGB 屏幕（1024x600 示例）**

```bash
cd /home/xuning/poky-rk3506-evm/poky
source oe-init-build-env

# 编辑 conf/local.conf，添加或修改：
MACHINE = "rockchip-rk3506-evb-rgb"

# 构建
bitbake core-image-minimal
```

---

## ⚠️ RGB 屏幕使用前必读

RGB 设备树文件中的参数是 **示例值**，需要根据你的实际屏幕修改！

### **必须修改的文件**

```
meta-rockchip/recipes-kernel/linux/files/rk3506g-evb1-v10-rgb.dts
```

### **必须修改的参数**

1. **GPIO 引脚** (根据硬件连接):
   ```dts
   enable-gpios = <&gpio0 RK_PA1 GPIO_ACTIVE_LOW>;
   reset-gpios = <&gpio0 RK_PA7 GPIO_ACTIVE_LOW>;
   ```

2. **显示时序** (从屏幕 Datasheet 获取):
   ```dts
   clock-frequency = <51200000>;  /* 像素时钟 */
   hactive = <1024>;              /* 分辨率宽度 */
   vactive = <600>;               /* 分辨率高度 */
   /* ... 其他时序参数 */
   ```

---

## 📁 已创建的文件

```
meta-rockchip/
├── conf/machine/
│   ├── rockchip-rk3506-evb-dsi.conf  ✅ 新建
│   └── rockchip-rk3506-evb-rgb.conf  ✅ 新建
│
├── recipes-kernel/linux/
│   ├── linux-rockchip_6.6.bb         ✅ 已修改
│   └── files/
│       ├── rk3506g-evb1-v10-dsi.dts  ✅ 新建
│       ├── rk3506g-evb1-v10-rgb.dts  ✅ 新建
│       └── drm.cfg                   ✅ 已修改
│
└── docs/
    └── RK3506-DISPLAY-CONFIGURATION.md  ✅ 详细文档
```

---

## 🔄 切换显示屏类型

只需修改 `build/conf/local.conf` 中的 MACHINE 变量：

```bash
# 从 DSI 切换到 RGB
MACHINE = "rockchip-rk3506-evb-rgb"

# 清理并重新构建
bitbake core-image-minimal -c cleanall
bitbake core-image-minimal
```

---

## 📖 详细文档

完整配置说明请查看：
```
meta-rockchip/docs/RK3506-DISPLAY-CONFIGURATION.md
```

---

## ✅ 验证清单

使用 RGB 屏幕前请确认：

- [ ] 已获取屏幕的 Datasheet
- [ ] 已确认硬件 GPIO 引脚连接
- [ ] 已修改设备树中的 GPIO 配置
- [ ] 已修改设备树中的时序参数
- [ ] 已确认没有引脚冲突（与 SD/I2C/SPI）
- [ ] 已确认屏幕电源供应 (3.3V)

---

## 🆘 遇到问题？

1. **DSI 屏幕无显示**: 检查 GPIO 和初始化序列
2. **RGB 屏幕无显示**: 检查时序参数和引脚连接
3. **花屏/闪烁**: 调整时序参数，特别是时钟频率

查看内核日志获取更多信息：
```bash
dmesg | grep -E "drm|panel|display"
```

---

**配置完成日期**: 2024-12  
**符合标准**: Yocto Project 最佳实践 ✅


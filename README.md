# RK3506 Yocto 项目

基于 Yocto Project 的 RK3506 嵌入式 Linux 开发环境。

## 硬件参数

**RK3506G**

| 项目 | 参数 |
|------|------|
| 处理器 | Rockchip RK3506G2 (3x Cortex-A7) |
| 主频 | 1.5GHz |
| 内存 | 128MB DDR3L 内置|
| 存储 | 256MB Nand Flash |
| 显示接口 | MIPI DSI 2 Lane (1280x1280@60Hz), RGB888 (1280x1280@60Hz) |

## meta-rockchip BSP 层

本项目使用 `meta-rockchip` BSP 层，这是专为 Rockchip SoC 开发板定制的 Yocto 层，提供：

- RK3506 设备树和内核配置
- U-Boot 引导加载程序支持
- DRM/KMS 显示驱动
- 多媒体和图形框架集成

更多信息请查看 `meta-rockchip/README.md`。

## 显示测试程序

在 `meta-rockchip/recipes-graphics/dsi-display-test` 目录下提供了多个 DSI 显示测试工具：

| 程序 | 说明 |
|------|------|
| `dsi-display-test` | 基础 DRM 显示测试，循环显示不同颜色 ✅|
| `st7701-panel-test` | 面板测试程序（通过设备树写入面板初始化序列） ✅|
| `lvgl-test` | LVGL 轻量级图形库演示 ✅|
| `mp4-player` | GStreamer 视频播放器（软解码） ✅|
| `pyqt-test` | PyQt5 图形界面测试程序 ✅ |

**注意**: RK3506G2 无硬件视频解码器，视频播放使用软件解码。

## 已知问题

- ⚠️ **串口波特率**: 当前为 1.5M (1500000)，暂时无法修改为 115200
- ⚠️ **触摸屏**: 触摸功能存在问题，待调试
- ⚠️ **设备树**: 仅完善了必要的基础设备树配置
- ⚠️ **PyQt5 字体**: 界面字体显示存在小问题

## 串口配置

使用串口调试工具连接设备时，必须设置波特率为 **1500000**：

```bash
# screen
screen /dev/ttyUSB0 1500000

# minicom
minicom -D /dev/ttyUSB0 -b 1500000
```

## 快速开始

构建镜像：

```bash
source poky/oe-init-build-env
bitbake core-image-minimal
```

详细构建说明请参考 `硬件相关/rk3506g构建命令.txt`。

## 致谢

特别感谢：

- [**Yocto Project**](https://www.yoctoproject.org/) 团队和社区，为嵌入式 Linux 开发提供了强大而灵活的构建系统和工具链
- **Jeffy Chen** (jeffy.chen@rock-chips.com) 维护的 [meta-rockchip](https://github.com/JeffyCN/meta-rockchip) BSP 层，为 Rockchip SoC 开发板提供了优秀的 Yocto 支持

## License

本项目遵循 MIT 许可证。

## 免责声明

**重要提示：**

本 Yocto 项目为实验性开发环境，主要用于 RK3506 平台的功能验证和技术探索。当前状态说明：

- ⚠️ 本项目**仅用于功能测试和开发调试**，未经充分的生产环境验证
- ⚠️ 不保证系统的稳定性、安全性和长期可靠性
- ⚠️ 是否具备实际生产部署能力需要根据具体应用场景进行全面测试和评估
- ⚠️ 使用本项目所产生的任何问题、损失或后果，作者不承担任何责任

**建议：**

- 在生产环境部署前，请进行充分的功能测试、压力测试和稳定性测试
- 根据实际需求完善设备树配置、驱动程序和系统安全设置
- 建议由专业团队评估后再决定是否用于商业产品

**本项目按"原样"提供，不附带任何明示或暗示的保证。使用者需自行承担使用风险。**

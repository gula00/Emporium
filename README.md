# Emporium 六场景功能重建

这是依据 Force Dimension SDK 3.17.7 中的 `Emporium` 二进制分析，使用 CHAI3D 官方示例制作的**可运行功能重建项目**。它不是原版源码，也尚未达到逐帧、逐参数一致。

## 场景来源

| 场景 | 本项目源文件 | CHAI3D 参考示例 | 本项目改动 |
|---|---|---|---|
| 1 Microscopic World | `src/scene1.cpp` | `examples/GLFW/20-map` | 高度图换成 SDK 的 `micro.png` |
| 2 Haptic Textures | `src/scene2.cpp` | `examples/GLFW/14-textures` | 使用 SDK 的 `sand.png` 等资源 |
| 3 Virtual Painting | `src/scene3.cpp` | `examples/GLFW/15-paint` | 使用 SDK 的 `palette.jpg`、`paper.jpg` |
| 4 Deformable Membrane | `src/scene4.cpp` | `modules/GEL/examples/GLFW/01-GEL-membrane` | 使用 SDK 的 `shadow.png` 和相同的 `box.obj` |
| 5 Dynamic Bodies | `src/scene5.cpp` | `modules/ODE/examples/GLFW/01-ODE-cube` | 根据二进制重建 5 个盒体、质量和初始位置，支持最多两个光标 |
| 6 Tactile Sensations | `src/scene6.cpp` | `examples/GLFW/11-effects` | 使用 SDK 中相同的球面贴图 |

原版 `Emporium` 是一个共享窗口、共享 UI、可在场景间切换的程序。本项目的启动器逐个运行六个独立场景，因此**场景切换方式尚未还原**。场景 1、2、3、4、6 主要保留 CHAI3D 示例的交互和参数；场景 5 的几何与物理参数还原更多，但灯光、材质、初始旋转和双设备附加力仍有差异。真实设备上的触觉手感尚未测试。

## 构建

当前工程面向 Windows x64、VS 2022，使用预先构建的 Release 版 CHAI3D、GEL、ODE 和 GLFW 静态库。默认目录布局：

```text
workspace/
  Emporium/             # 本仓库
  chai3d-src/           # https://github.com/chai3d/chai3d
  build-chai3d/         # Release/chai3d.lib 和 extras/GLFW/Release/glfw.lib
  build-gel/            # Release/chai3d-GEL.lib
  build-ode/            # Release/chai3d-ODE.lib
```

构建依赖库并配置本工程：

```powershell
cmake -S ../chai3d-src -B ../build-chai3d -G "Visual Studio 17 2022" -A x64 '-DCMAKE_POLICY_VERSION_MINIMUM=3.5'
cmake --build ../build-chai3d --config Release --target chai3d glfw --parallel 6

$chaiBuild = (Resolve-Path ../build-chai3d).Path
$glfwBuild = Join-Path $chaiBuild 'extras/GLFW'
cmake -S ../chai3d-src/modules/GEL -B ../build-gel -G "Visual Studio 17 2022" -A x64 '-DCMAKE_POLICY_VERSION_MINIMUM=3.5' "-Dchai3d_DIR=$chaiBuild" "-DGLFW_DIR=$glfwBuild"
cmake --build ../build-gel --config Release --target chai3d-GEL --parallel 6
cmake -S ../chai3d-src/modules/ODE -B ../build-ode -G "Visual Studio 17 2022" -A x64 '-DCMAKE_POLICY_VERSION_MINIMUM=3.5' "-Dchai3d_DIR=$chaiBuild" "-DGLFW_DIR=$glfwBuild"
cmake --build ../build-ode --config Release --target chai3d-ODE --parallel 6

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 6
```

目录布局不同时，配置时传入 `-DCHAI3D_ROOT=...`、`-DCHAI3D_BUILD=...`、`-DGEL_BUILD=...`、`-DODE_BUILD=...`。CHAI3D 源码、依赖库和机器专用的 `CMakeUserPresets.json` 均不提交到本仓库。

## 准备运行资源

资源和 SDK 二进制不随本仓库分发。安装 [Force Dimension SDK 3.17.7](https://www.forcedimension.com/software/sdk)，在有 Python 3 的环境中执行：

```powershell
python tools/prepare_resources.py --chai3d-root ../chai3d-src --sdk-root D:/sdk-3.17.7
```

脚本从本机 CHAI3D 和 SDK 安装中提取这六个场景实际需要的文件，写入被 Git 忽略的 `bin/resources`。Force Dimension 设备运行时还需要 SDK 的 `drd64.dll` 或 `dhd64.dll` 能被 CHAI3D 找到。没有设备时可以运行图形和物理部分，触觉效果无法实测。

## 运行

从 `bin/win-x64` 目录启动 `EmporiumReconstruction.exe`，输入 1–6 选择场景。也可执行 `EmporiumReconstruction.exe --scene 5`。场景窗口按 `q` 关闭后返回菜单；每个示例的其他按键请看启动后的控制台提示。

## 来源与许可

CHAI3D 示例保留原始文件头中的 BSD 许可文字，完整许可副本在 `CHAI3D-copyright.txt`。本地准备的资源仍遵循各自来源的许可；本仓库不包含 SDK 随包资源、SDK 库或原版 `Emporium.exe`。

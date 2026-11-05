# Cornix 的 ZMK 固件（日本定制版）

**语言:** [English](./README.md) | [日本語](./README_ja.md) | 中文

本仓库（`numachang/cornix-zmk-custom`）是
[`hitsmaxft/zmk-keyboard-cornix`](https://github.com/hitsmaxft/zmk-keyboard-cornix) 的**面向日本的下游分支**，
包含 Cornix 分体式键盘的 ZMK 固件配置以及开发板/扩展板模块。

由于在日本 Cornix 是作为**普通的蓝牙分体式键盘**（无适配器/dongle）销售和使用的，本分支有意在以下几个方面与上游不同：

- **禁用了适配器（dongle）构建目标。** 键盘以标准分体方式运行，右半部分作为中心（central）。适配器相关扩展板仍保留在
  磁盘上，但不包含在有效的构建矩阵中。
- **在 `cornix_right` 上启用了 ZMK Studio**，可通过 USB 使用
  [ZMK Studio](https://zmk.dev/docs/features/studio) 实时编辑键位图。
- **将 RGB 指示灯（`cornix_indicator`）** 内置到有效的构建目标中。

> ### 在找 RMK 固件吗？
> Cornix 的原厂固件是 **RMK**。对应的 RMK 定制版本在此维护：
> **https://github.com/numachang/cornix-rmk-custom**
>
> 想继续使用 RMK 就用那个仓库；想使用 ZMK 就用*本仓库*。

> ### 编辑键位图：ZMK Studio Tweaks（定制 Web 编辑器）
> ZMK Studio 的改版部署在 **https://zmk-studio-tweaks.numachang.com/**
>（源码：**https://github.com/numachang/zmk-studio-tweaks**）。它通过 Web Serial API 完全在 Chromium 浏览器中运行
>（数据不会离开你的电脑），可直接打开 `cornix_right` 并在其中编辑键位图。
>
> 它是官方 ZMK Studio 的一个分支，在上游编辑器的基础上增加了：
> - **键位图导入/导出**（JSON 文件），并提供来自固件的错误反馈。
> - **可视化按键选择器** — 带搜索的网格式 HID 选择器、分类的 behavior，以及可点击的物理配列渲染
>   （ANSI/ISO/**JIS**）。
> - **主机配列本地化** — 支持包括**日语**在内的 22 种 OS 配列。在不改变底层 HID 绑定的前提下，
>   选择器和标签会随你的键盘配列自适应显示。
>
> 这里用于在不受上游评审周期阻塞的情况下原型化功能，意图是把能干净落地的功能以 PR 形式回馈上游。
> 它**与 ZMK 项目无关，也未获其认可**。如需稳定且受支持的使用，请优先选择[官方 ZMK Studio](https://zmk.studio/)。

![image](images/cornix_with_dongle.png)
![image](images/cornix_layout.png)

## 开发板和扩展板

### 开发板

- **`cornix_left`**: Cornix 分体式键盘的左半部分。作为外围（从）侧。
- **`cornix_right`**: Cornix 分体式键盘的右半部分。作为中心（主）侧。此处启用了 ZMK Studio。
- **`cornix_ph_left`**: 用于适配器配置的备用左半部分开发板。**本分支未使用**（为与上游保持一致而保留）。

### 扩展板

- **`cornix_indicator`**: 启用 RGB LED 指示灯以显示电池和连接状态。会消耗更多电量。
- **`cornix_dongle_adapter`** / **`cornix_dongle_eyelash`**: 来自上游的适配器相关扩展板。它们存在于磁盘上，但在本分支中
  **不被有效的构建矩阵引用**。

这个社区固件已在使用 ZMK 的 Cornix 上进行了测试，并按照 ZMK 分体式指南提供完整的分体角色配置、电池电源管理以及
蓝牙中心/外围设置。

## 支持的硬件：Cornix 分体式键盘

Cornix Split Tented Low-Profile Ergo Keyboard (Jezail Funder)

Cornix 是一款受 Corne 启发的分体式人体工学键盘，具有紧凑的 3×6 列交错布局和六个拇指集群键（每半边三个）。它提供
10°、18° 和 25° 的可调帐篷角度，帮助用户减少手腕压力并找到自定义的人体工学对齐。

- **分体式、列交错布局**（3×6 + 拇指集群）。
- **可调帐篷支撑** 10°、18°、25°（基于硬件，无需固件黑科技）。
- **Kailh Choc V2 热插拔插座**，支持 LAK 或 LCK 低轮廓键帽。
- **双模连接**：有线 USB-C 或蓝牙无线（右半边为中心）。
- **固件**：原厂固件为 RMK（支持 VIAL）；本仓库提供 ZMK 替代方案。
- 高端 **CNC 加工铝制底盘**、定制减震泡沫和便携存储袋。

> 上游项目所有者也是 RMK 贡献者，请支持 RMK https://rmk.rs/

## Bootloader / 恢复说明

自开发板 **v2.3** 起，闪存分区布局已更新，SoftDevice 分区被缩小（150K → 4K），因此可以直接刷写 `.uf2` 文件，
**无需恢复 SoftDevice**。

- 如果你从非常旧的固件迁移，可能需要先用 `reset.uf2` 重置。
- 你可以通过刷写原始 `.uf2` 文件回滚到原厂 RMK 固件；备份文件位于 `rmkfw/`。
- 更深入的 bootloader 恢复说明（旧的 SoftDevice 恢复方式）见
  [bootloader/README.md](./bootloader/README.md)。

## 🔰 简单方法：克隆此仓库并使用 GitHub Actions 构建

如果你是 ZMK 新手，不想处理 `west.yml` 或模块管理，可以直接使用此仓库来定制固件。

### 步骤

1. **Fork 或克隆此仓库**
   - 点击右上角的 **Fork** 将仓库复制到你的 GitHub 账户，或在本地运行 `git clone`。

   > 推荐 Fork，因为 GitHub Actions 会自动构建你的固件。

2. **编辑你的键位图**
   - 键位图位于 `config/cornix.keymap`。可直接编辑，或使用 ZMK Studio（在 `cornix_right` 上启用）—
     [官方编辑器](https://zmk.studio/) 或改版
     [zmk-studio-tweaks.numachang.com](https://zmk-studio-tweaks.numachang.com/)（见上文）— 或
     [ZMK 键位图编辑器](https://nickcoutsos.github.io/keymap-editor/)。
   - 提交并推送到 GitHub。

3. **使用 GitHub Actions 构建**
   - 推送 `boards/*` 或 `config/*` 下的更改会自动触发构建。你也可以从 **Actions → “Build ZMK firmware” →
     Run workflow** 手动启动（仅修改 `build.yaml` 不会自动触发构建，此时请手动运行）。
   - 工作流完成后，转到 **Actions → 最新运行 → Artifacts** 下载固件（`firmware.zip`，每个目标包含一个 `.uf2`）。

4. **刷写键盘**
   - 将每一半置于 UF2 引导加载程序模式（通常通过双击复位按钮）。
   - 将对应的 `.uf2` 拖放到挂载的驱动器上。
   - 使用**支持数据传输的 USB-C 数据线**，并**从同一次构建刷写两半**，以避免分体协议不匹配。

### 适用于谁？

- ZMK 新手
- 只想自定义键位图的用户
- 不需要修改驱动程序或硬件定义的人

## 构建目标（`build.yaml`）

本分支提供以下有效目标（适配器目标已注释）：

```yaml
include:
  # 左半部分（外围）— RGB 指示灯
  - board: cornix_left
    shield: cornix_indicator
    artifact-name: cornix_left_default_nosd

  # 右半部分（中心）— RGB 指示灯 + 通过 USB 的 ZMK Studio
  - board: cornix_right
    shield: cornix_indicator
    snippet: studio-rpc-usb-uart nrf52840-nosd
    artifact-name: cornix_right_nosd

  # 设置重置（重新配对时刷写到两半）
  - board: cornix_right
    shield: settings_reset
    snippet: studio-rpc-usb-uart nrf52840-nosd
    artifact-name: cornix_reset
```

> [!NOTE]
> - `cornix_indicator` 扩展板启用 RGB LED，但会消耗更多电量。如果不需要指示灯，可从目标中移除它。
> - 重新配对步骤：向两侧刷写 `cornix_reset`，然后刷写 `cornix_left` 和 `cornix_right`，并同时复位两侧。

## 如何将 Cornix 模块添加到现有 ZMK 项目

对于已有 zmk-config 的用户，通过 `west.yml` 将此仓库添加为依赖，并用 `west update` 拉取。

### 1. 修改 `west.yml`

在 `manifest/remotes` 部分添加：

```yaml
remotes:
  - name: zmkfirmware
    url-base: https://github.com/zmkfirmware
  - name: numachang
    url-base: https://github.com/numachang
  - name: urob
    url-base: https://github.com/urob
```

在 `manifest/projects` 部分添加：

```yaml
projects:
  - name: zmk
    remote: zmkfirmware
    revision: main
    import: app/west.yml
  - name: cornix-zmk-custom
    remote: numachang
    revision: main
  - name: zmk-helpers
    remote: urob
    revision: main
```

> 想使用上游而非日本分支？请使用 `url-base: https://github.com/hitsmaxft`，项目名为 `zmk-keyboard-cornix`。

### 2. 更新依赖

```bash
west update
```

### 3. 配置构建

将[构建目标](#构建目标buildyaml)中的目标添加到你的 `build.yaml`。

### 4. 构建并刷写

- 无需恢复 SoftDevice（自开发板 v2.3 起）。
- 如需重新配对，向两侧刷写 `cornix_reset`。
- 刷写左右 `.uf2` 文件并同时复位两侧。

## 本地构建此项目（不使用 `west.yml` 依赖）

如果你更喜欢在不将其添加为 `west.yml` 依赖的情况下本地构建，可以使用 `ZMK_EXTRA_MODULES` cmake 参数。

### 先决条件

1. 一个可用的 ZMK 开发环境。
2. 已将此仓库克隆到本地目录。

### 构建步骤

1. **克隆此仓库**：
   ```bash
   git clone https://github.com/numachang/cornix-zmk-custom.git
   ```

2. **使用额外模块配置你的 ZMK 构建** — 编辑 `.west/config` 并在 `[build]` 部分添加 cmake 参数：

   ```ini
   [build]
   cmake-args = -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DZMK_EXTRA_MODULES=/full/absolute/path/to/cornix-zmk-custom
   ```

   将路径替换为你克隆此仓库的实际绝对路径。

3. **构建固件**：
   ```bash
   west build -b cornix_left
   west build -b cornix_right
   ```

此方法允许你在不修改现有 ZMK 配置的 `west.yml` 的情况下使用 Cornix 模块。

### 使用 Nix 进行本地构建（Linux / macOS）

通过 `Justfile` 提供了基于 Nix 的流程：

```bash
nix develop          # 提供 zephyr-sdk、west、just、yq、keymap-drawer
just init            # west init -l config && west update && west zephyr-export
just list            # 显示从 build.yaml 解析出的构建目标
just build cornix_left
just clean
just draw cornix     # 通过 keymap-drawer 渲染键位图 SVG
```

Windows 用户应改用 GitHub Actions。

## 关于 RGB

Cornix 扩展板每侧有 2 个 RGB LED，在原厂固件中由 PWM 控制。在 ZMK 中，这些 LED 由 `cornix_indicator` 扩展板驱动，
用于指示电池和连接状态。完全原生的树外 RGB 模块也在计划中（见 issue #2）。

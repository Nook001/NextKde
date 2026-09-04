# KOS Desktop Shell - NixOS 打包对话记录

## 背景

将 KOS Desktop Shell（基于 Quickshell 的 iPadOS 风格桌面环境，运行于 KDE Plasma 6）打包为 NixOS 软件包。

## 项目结构

Nextkde 项目包含以下组件：
- **shell-data-service**：Go 数据服务（系统指标、活动账本、桌面文件监听）
- **file-clipboard-helper**：Qt 剪贴板辅助器
- **kwin-window-bridge**：C++ D-Bus 窗口桥
- **kos-settings**：设置应用（C++/QML）
- **kwin-effects-glass**：KWin 模糊/折射特效
- **kwin-dock-window-animation**：iPadOS 风格窗口动画
- **kwin-context-menu-input**：右键菜单外部点击关闭
- **QML Shell 配置**：Quickshell 直接解释运行

## 打包过程

### 1. 创建 Nix 包定义

在 `nix-configuration-xiaoyintx/package/kos-desktop/` 下创建：
- `default.nix` - 整合包
- `shell-data-service.nix` - Go 数据服务
- `kwin-window-bridge.nix` - KWin 窗口桥
- `kos-settings.nix` - 设置应用
- `kwin-dock-window-animation.nix` - 窗口动画
- `kwin-context-menu-input.nix` - 右键菜单特效

### 2. 遇到的问题与解决方案

| 问题 | 解决方案 |
|------|---------|
| `callPackage` 不能作为函数参数被自动注入 | 改用 `pkgs` 参数，通过 `pkgs.callPackage` 调用子包 |
| `src` 与 nixpkgs 中已重命名的包冲突 | 改名 `kosSrc` |
| `lib.fakeSha256` 格式不对 | 改用 `lib.fakeHash` |
| `extra-cmake-modules` 已移除 Qt5 版本 | 用 `kdePackages.extra-cmake-modules` |
| CMake 缺少 `install()` target | 自定义 `installPhase` |
| Qt wrapping 错误 | 设置 `dontWrapQtApps = true` |
| Go 模块下载超时 | 添加 `GOPROXY=https://goproxy.cn,direct` |
| symlink 路径错误 | 修正为实际安装路径（`lib/quickshell/`、`libexec/`） |
| 纯求值模式不允许绝对路径 | 改用远程 GitHub 仓库作为 flake input |

### 3. 最终方案

**Nextkde 项目**：添加 `flake.nix` + `nix/` 目录，可被其他 flake 作为 input 引用。

**nix-configuration-xiaoyintx**：
- `flake.nix` 中添加 `kos-desktop = { url = "github:SuceV587/NextKde"; };`
- overlay 中用 `kosSrc = kos-desktop` 传入源码
- 所有包通过 `pkgs.localpkg.kos-desktop` 访问

## 构建与使用

### 构建

```bash
cd ~/nix-configuration-xiaoyintx
nix flake lock --update-input kos-desktop
sudo nixos-rebuild switch --flake .#omen-16
```

### 启用

```bash
# 启动 Shell
quickshell --path /run/current-system/share/kos-desktop

# 启用数据服务
systemctl --user enable --now shell-data-service.service

# 设置全局快捷键
python3 /run/current-system/share/kos-desktop/helpers/global-shortcuts/install.py
```

### KWin 特效

在「系统设置 → 桌面特效」中启用：
- `blurplus`
- `kos-dock-window-animation`
- `quickshell-context-menu-input`

### 通知接管

从 Plasma 托盘移除通知组件，然后：
```bash
systemctl --user restart plasma-plasmashell
```

### 默认快捷键

| 快捷键 | 功能 |
|--------|------|
| `Meta+Space` | 应用启动器 |
| `Meta+Shift+Space` | 窗口切换器 |
| `Meta+B` | 控制中心 |
| `Meta+Tab` | 工作区概览 |

### 恢复 Plasma Shell

```bash
systemctl --user restart plasma-plasmashell
```

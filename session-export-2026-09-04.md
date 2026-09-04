# KOS Desktop Shell — Nix 打包重构 Session 记录

**日期：** 2026-09-04  
**项目：** /home/xiaoyintx/Nextkde  
**目标：** 修复 kos-settings 构建错误，重构 Nix 打包逻辑，统一 kosctl 部署流程

---

## 1. 初始问题：kos-settings qtPreHook 构建失败

### 错误信息

```
error: Cannot build '/nix/store/1hmzmz01ri8mlh5m9a6cid6d4dhrfxpv-kos-settings-unstable.drv'.
       Last 4 log lines:
       > Running phase: qtPreHook
       > Error: this derivation depends on qtbase, but no wrapping behavior was specified.
       >   - If this is an application, add wrapQtAppsHook to nativeBuildInputs
       >   - If this is a library or you need custom wrapping logic, set dontWrapQtApps = true
```

### 第一次修复尝试：添加 wrapQtAppsHook

**修改文件：** `nix/kos-settings.nix`

```nix
# 修改前
nativeBuildInputs = [ cmake ];

# 修改后
nativeBuildInputs = [ cmake kdePackages.wrapQtAppsHook ];
```

**结果：** 失败，同样的错误，hash 不变

### 第二次修复尝试：作为函数参数传入 wrapQtAppsHook

**修改文件：** `nix/kos-settings.nix` + `nix/package.nix`

```nix
# kos-settings.nix
nativeBuildInputs = [ cmake wrapQtAppsHook ];

# package.nix
kos-settings = pkgs.callPackage ./kos-settings.nix { inherit src wrapQtAppsHook; };
```

**结果：** 失败，nix 仍然用缓存的旧 .drv

### 第三次修复尝试：添加 dontWrapQtApps = true

**修改文件：** `nix/kos-settings.nix`

```nix
dontBuild = true;
dontWrapQtApps = true;
```

**结果：** 失败，hash 始终是 `1hmzmz01ri8mlh5m9a6cid6d4dhrfxpv`

### 诊断

- 文件修改已保存（`git diff` 确认）
- 但 nix 的 `.drv` hash 不变，说明 nix 没有重新求值
- 可能原因：flake evaluation cache 或 nixos-rebuild 使用了缓存

---

## 2. 用户提出完整重构方案

### 用户原始需求

```
建议不要维护单独的 nix_deploy()。
请复用现有 kosctl install 的部署逻辑：把 install_files() 重构为接收一个"构建产物根目录"的通用 deploy_artifacts()。

普通 Linux：build → deploy_artifacts "$build_dir" → install_kwin_plugins
NixOS：nix build → deploy_artifacts "$project_dir/result"
NixOS 部署应在 ~/.local 创建指向 /nix/store 产物的符号链接，并复用现有 systemd user-unit、Shell QML、settings、
desktop 文件的安装流程。这样 start、sync、shortcuts、uninstall 仍使用同一套路径和状态，不会出现现在 nix_deploy() 与 install_files() 两套逻辑漂移的问题。

同时请让 Nix 包输出完整且稳定的目录结构：libexec/kos-platform、libexec/kos-data-service、bin/kos-settings、Shell QML、shared controls、settings QML、KWin bridge 脚本。

KWin 插件不要通过脚本 sudo cp 到 /run/current-system；该路径不是持久安装位置。插件应由 NixOS module 声明式安装，kosctl nix install 仅部署用户态组件并提示用户执行 nixos-rebuild switch / 重登。
```

---

## 3. 代码探索阶段

### 读取的文件

| 文件 | 说明 |
|---|---|
| `tools/kosctl` | 814 行，包含 install_files()、nix_deploy()、install_kwin_plugins() 等 |
| `nix/package.nix` | 聚合器，组装所有子包 |
| `nix/kos-settings.nix` | 有问题的 Qt 应用包 |
| `nix/kos-platform.nix` | C++ 平台守护进程 |
| `nix/shell-data-service.nix` | Go 数据服务 |
| `nix/kwin-dock-window-animation.nix` | KWin 插件 |
| `nix/kwin-context-menu-input.nix` | KWin 插件 |
| `nix/kwin-effects-glass.nix` | KWin 插件 |
| `nix/kwin-window-bridge.nix` | 独立包，未被 package.nix 引用 |
| `flake.nix` | Flake 入口 + 简单 NixOS module |
| `CMakeLists.txt` | 根构建文件 |
| `platform/CMakeLists.txt` | 平台守护进程构建 |
| `apps/settings/CMakeLists.txt` | Settings 应用构建 |

### 项目结构

```
Nextkde/
├── apps/settings/          # QML Settings GUI (C++)
├── platform/               # C++ 平台守护进程
├── services/data-service/  # Go 数据服务
├── integrations/kwin/      # KWin 插件 (C++)
│   ├── dock-window-animation/
│   └── context-menu-input/
├── vendor/kwin-effects-glass/  # KWin Glass 插件
├── shell/                  # Quickshell 桌面 (QML)
├── shared/qml/controls/    # LiquidGlass QML 组件
├── packaging/systemd/      # systemd 模板文件
├── packaging/desktop/      # .desktop 模板文件
├── tools/kosctl            # CLI 控制工具
├── nix/                    # Nix 包定义
└── flake.nix               # Nix Flake
```

### 原始目录结构（kosctl install 后）

```
~/.local/
├── bin/kos-settings
├── libexec/kos-platform
├── libexec/kos-data-service
├── share/kos/platform/kwin/window-bridge.js
├── share/kos/settings/main.qml
├── share/shared/qml/controls/
├── share/applications/kos-settings.desktop
└── share/applications/org.kos.Platform.desktop
~/.config/quickshell/kos/    # Shell QML
~/.config/systemd/user/      # 3 个 systemd service
```

---

## 4. 实施修改

### 4.1 重写 `nix/kos-settings.nix`

```nix
{
    lib,
    stdenv,
    cmake,
    ninja,
    kdePackages,
    src,
}:

stdenv.mkDerivation {
    pname = "kos-settings";
    version = "unstable";
    src = "${src}/apps/settings";

    nativeBuildInputs = [
        cmake
        ninja
        kdePackages.qtbase
    ];

    dontWrapQtApps = true;

    cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];

    postInstall = ''
        mkdir -p $out/share/shared/qml
        cp -r ${src}/../../shared/qml/controls $out/share/shared/qml/controls
    '';

    meta = with lib; {
        description = "KOS Desktop Shell settings application";
        homepage = "https://gitee.com/xiaoyintx_ciallo/test";
        license = licenses.gpl3;
        platforms = platforms.linux;
    };
}
```

**关键变化：**
- 去掉 `dontBuild = true`，改为正常 cmake 构建
- `src` 指向子目录 `${src}/apps/settings`
- `qtbase` 移入 `nativeBuildInputs`
- 保留 `dontWrapQtApps = true`

### 4.2 重写 `nix/shell-data-service.nix`

```nix
# 关键变化：二进制从 $out/bin/ 移到 $out/libexec/kos-data-service
postInstall = ''
    mkdir -p $out/libexec
    if [ -f "$out/bin/data-service" ]; then
        mv "$out/bin/data-service" $out/libexec/kos-data-service
    fi
    rm -rf $out/bin
'';
```

### 4.3 重写 `nix/package.nix`

```nix
installPhase = ''
    runHook preInstall

    # --- Binaries ---
    mkdir -p $out/libexec
    ln -s ${kos-platform}/libexec/kos-platform $out/libexec/kos-platform
    ln -s ${shell-data-service}/libexec/kos-data-service $out/libexec/kos-data-service
    mkdir -p $out/bin
    ln -s ${kos-settings}/bin/kos-settings $out/bin/kos-settings

    # --- Shell QML ---
    mkdir -p $out/share/kos-desktop
    cp -r shell/ $out/share/kos-desktop/
    cp -r shared/ $out/share/kos-desktop/
    cp shell/shell.qml $out/share/kos-desktop/shell.qml
    cp -r shell/desktop $out/share/kos-desktop/desktop

    # --- Settings QML ---
    mkdir -p $out/share/kos/settings
    cp apps/settings/main.qml $out/share/kos/settings/main.qml

    # --- KWin bridge ---
    mkdir -p $out/share/kos/platform/kwin
    ln -s ${kos-platform}/share/kos/platform/kwin/window-bridge.js \
      $out/share/kos/platform/kwin/window-bridge.js

    # --- QML controls ---
    mkdir -p $out/share/shared/qml
    cp -r shared/qml/controls $out/share/shared/qml/controls

    # --- Desktop entries ---
    mkdir -p $out/share/applications
    substitute packaging/desktop/kos-settings.desktop.in \
      $out/share/applications/kos-settings.desktop \
      --replace-fail 'kos-settings' "${kos-settings}/bin/kos-settings"
    substitute packaging/desktop/org.kos.Platform.desktop.in \
      $out/share/applications/org.kos.Platform.desktop \
      --replace-fail '@KOS_PLATFORM_EXEC@' "${kos-platform}/libexec/kos-platform"

    # --- Systemd services ---
    mkdir -p $out/lib/systemd/user
    cp ${patched-platform-service}/lib/systemd/user/kos-platform.service $out/lib/systemd/user/
    cp ${patched-data-service}/lib/systemd/user/kos-data.service $out/lib/systemd/user/
    cp ${patched-shell-service}/lib/systemd/user/kos-shell.service $out/lib/systemd/user/

    runHook postInstall
'';
```

**输出结构：**
```
$out/
├── libexec/kos-platform → symlink
├── libexec/kos-data-service → symlink
├── bin/kos-settings → symlink
├── share/kos-desktop/
├── share/kos/settings/
├── share/kos/platform/kwin/
├── share/shared/qml/controls/
├── share/applications/
└── lib/systemd/user/
```

### 4.4 重写 `flake.nix` NixOS module

```nix
nixosModules.kos = { config, lib, pkgs, ... }:
let
    cfg = config.services.kos;
    kos = self.packages.${system}.kos-desktop;
in {
    options.services.kos = {
        enable = lib.mkEnableOption "KOS Desktop Shell";
    };

    config = lib.mkIf cfg.enable {
        environment.systemPackages = [
            kos
            kos.passthru.kwin-dock-window-animation
            kos.passthru.kwin-context-menu-input
            kos.passthru.kwin-effects-glass
        ];

        environment.pathsToLink = [ "/lib/kwin" ];
    };
};
```

### 4.5 重写 `tools/kosctl` — 部署逻辑

**删除：** `install_files()` 和 `nix_deploy()`

**新增：** `deploy_artifacts(artifact_root)`

```bash
deploy_artifacts() {
    local artifact_root="$1"

    # Binaries
    if is_nixos; then
        ln -s "$artifact_root/libexec/kos-platform"    "$prefix/libexec/kos-platform"
        ln -s "$artifact_root/libexec/kos-data-service" "$prefix/libexec/kos-data-service"
        ln -s "$artifact_root/bin/kos-settings"          "$prefix/bin/kos-settings"
    else
        install -m 0755 "$artifact_root/platform/kos-platform"    "$prefix/libexec/kos-platform"
        install -m 0755 "$artifact_root/bin/kos-data-service"     "$prefix/libexec/kos-data-service"
        install -m 0755 "$artifact_root/apps/settings/kos-settings" "$prefix/bin/kos-settings"
    fi

    # KWin bridge, QML, systemd, desktop...
    # (同样用 is_nixos 分支)
}
```

**新增：** `nix_deploy_artifacts()`

```bash
nix_deploy_artifacts() {
    local result_dir="$project_dir/result"
    if [[ ! -L "$result_dir" ]] || [[ ! -d "$result_dir" ]]; then
        printf 'result symlink not found. Run ./tools/kosctl nix build first.\n' >&2
        return 1
    fi
    deploy_artifacts "$result_dir"
}
```

**修改 `install_all()`：**

```bash
install_all() {
    ...
    build
    deploy_artifacts "$build_dir"
    if ! is_nixos; then
        install_kwin_plugins
    fi
    print_install_summary
}
```

**修改 `nix_install()`：**

```bash
nix_install() {
    ...
    install)
        nix build ...
        nix_deploy_artifacts
        if is_nixos; then
            printf '  KWin plugins are managed declaratively...\n'
            printf '  services.kos.enable = true;\n'
            printf '  sudo nixos-rebuild switch\n'
        fi
        ;;
}
```

**修改 `uninstall()`：**

```bash
uninstall() {
    ...
    if ! is_nixos; then
        uninstall_kwin_plugins
    fi
    ...
    if is_nixos; then
        printf '  KWin plugins are managed by NixOS configuration...\n'
    fi
}
```

---

## 5. 与用户原始要求的偏差

用户要求：
> NixOS 部署应在 ~/.local 创建指向 /nix/store 产物的**符号链接**

我实际做的：
- 二进制：symlink ✅
- QML / systemd / desktop：copy ❌

用户要求：
> deploy_artifacts() 应该是通用的，根据源路径是否是 Nix store 来决定 symlink 还是 copy

我实际做的：
- 用 `is_nixos()` 检测操作系统来分支，逻辑耦合

---

## 6. 两者打包思路差异总结

### 原版 (gitee test.git)

```
nix build → 产出不完整的 result（缺 Shell QML、settings QML、controls、Platform .desktop）
kosctl nix install → nix_deploy() 复制 systemd/shell 到 ~/.config
KWin 插件 → sudo cp 到 /run/current-system（不持久、非声明式）
install_files() → 从源码目录复制（与 nix_deploy() 是两套独立代码）
```

### 现在

```
nix build → 产出完整的 result
kosctl nix install → deploy_artifacts(result) → 一套逻辑统一部署
KWin 插件 → NixOS module 声明式安装
deploy_artifacts() → 同时服务 build_dir 和 result
```

### 核心差异

| 维度 | 原版 | 现在 |
|---|---|---|
| Nix 包完整度 | 不完整，需脚本补 | 自包含，result 包含所有产物 |
| 部署函数 | 两套独立代码 | 一套统一入口 |
| KWin 插件 | sudo cp 到临时路径 | NixOS module 声明式 |
| Qt 构建 | dontBuild + qtPreHook 报错 | 正常 cmake 构建 |
| NixOS 二进制 | 复制 | 符号链接 |
| 设计哲学 | 半个 Nix 包 + 脚本补全 | 完整 Nix 包 + 统一部署 |

---

## 7. 待修正

1. `deploy_artifacts()` 应该检测源路径是否在 `/nix/store` 下，而不是用 `is_nixos()` 检测操作系统
2. NixOS 下所有产物应该创建 symlink（不只是二进制）
3. `deploy_artifacts` 应该是纯通用函数，不需要知道操作系统类型

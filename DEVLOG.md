# WaveRider Developer Log

> 面向开发者的项目进度文档，由 Claude 在每个任务完成后更新。

**最后更新**: 2026-06-16

---

## 项目概览

| 项 | 值 |
|----|-----|
| 类型 | Windows 本地音频播放器 |
| 技术栈 | Qt 5.15.2 (MSVC) + BASS 2.4 + CMake |
| 语言 | C++17 |
| 平台 | Windows 11 x64 |
| 风格 | Phigros 极简美学，全自定义 QPainter 绘制 |
| Git | https://github.com/liwufan-xizi/WaveRider |

---

## 当前进度

### ✅ 已完成模块

| 模块 | 说明 | 关键技术点 |
|------|------|-----------|
| **音频引擎** | BASS 2.4 封装 | `BASS_StreamCreateFile` + Unicode 路径，100ms 位置轮询，无缝切歌 |
| **格式支持** | MP3 / WAV / FLAC | bass.dll 内置，已验证；OGG/AAC 未测试 |
| **播放列表** | M3U + 4 种播放模式 | `QAbstractListModel`，拖放排序，文件夹递归扫描 |
| **歌词系统** | LRC + 在线爬虫 | 三级加载（本地.lrc → 缓存 → lrclib.net API），编码回退 |
| **频谱可视化** | FFT1024 → 64 bars | 对数映射，非对称 EMA 平滑，峰值落点 + 径向发光 |
| **主题系统** | QSS 皮肤 + 动态配色 | `SkinManager`（@vars 变量替换）+ `ThemeConfig`（单例配色） |
| **背景系统** | 5 种显示模式 | `BackgroundManager` → `ColorExtractor`（HSV 主色提取）→ 全局配色自动跟随 |
| **收藏系统** | JSON 持久化 | `FavoritesManager` + 右侧滑动面板，Ctrl+F 快捷键 |
| **DSP 系统** | 内置效果 + 插件框架 | 10 段参量 EQ (±15dB, 8 预设) + 压缩器 + 混响，`DSPChain` 管理 |
| **UI 控件** | 全部自定义 QPainter | 菱形按钮、细进度条、音量条、心形收藏、汉堡菜单 |
| **PlaylistPanel** | 右侧 320px 滑动面板 | `QPropertyAnimation` 250ms OutCubic，半透明遮罩，搜索过滤 |
| **PlaylistDelegate** | 48px 自定义行 | 双行布局，当前播放指示条，收藏心形，ThemeConfig 配色 |
| **FavoritesPanel** | 同款滑动面板 | 搜索、Delete 移除、双击播放 |
| **DSPPanel** | 480px 浮动面板 | Ctrl+D 切换，主旁通开关，预设按钮行 |
| **EqualizerWidget** | 10 段自定义滑块 | click-to-set / drag / wheel ±0.5dB，hover 发光 |
| **SkinSelectorDialog** | 皮肤选择弹窗 360×280 | 解析 @vars 6 色预览，2 列网格，点击即用 |
| **BackgroundDialog** | 背景设置面板 400×320 | 144×96 预览，5 模式按钮，Blur/Dim 实时滑块 |
| **DSPPluginLoader** | 外部插件加载框架 | 扫描 DLL + JSON manifest 解析 + 工厂函数验证 + QLibrary 实例化 |
| **动态缩放** | Baseline 800×450，scale [0.8, 2.0] | `TrackInfoWidget` + `PlayerControlBar` 字号/布局全缩放 |

### 🔄 待做

| 优先级 | 任务 | 说明 |
|--------|------|------|
| P1 | EqualizerWidget 预设持久化 | 用户自定义 EQ 预设保存/加载 |
| P2 | 在线歌词源扩展 | 多数据源 fallback，歌词时间轴微调 |
| P3 | 内置主题 | 后期协商具体风格 |

---

## 架构要点

### SignalBus 解耦模式

```
AudioEngine ──emit──→ SignalBus ──listen──→ UI 面板
PlaylistManager ──emit──→ SignalBus ──listen──→ MainWindow
BackgroundManager ──emit──→ SignalBus ──listen──→ MainWindow
```

规则：`core/` 不依赖任何模块；`ui/` 可依赖所有模块；其他模块仅依赖 `core/`。

### 配色数据流

```
用户换背景 → BackgroundManager::setBackground()
  → ColorExtractor::extract()  (HSV 色相直方图)
  → ThemeConfig::setColors()
  → emit themeColorsChanged()
  → 所有自定义控件 repaint()
```

### DSP 数据流

```
AudioEngine::loadFile → new HSTREAM
MainWindow::onTrackStarted → DSPChain::setStream(newStream)
                           → reapplyAll() → 逐个 applyToStream()
DSPPanel UI → EqualizerWidget::bandChanged → EqualizerEffect::setBandGain()
            → BASS_FXSetParameters() 实时生效
```

---

## 已知问题 & 已修复

### 已修复

| 日期 | 问题 | 根因 | 修复 |
|------|------|------|------|
| 2026-06-16 | 播放 MP3 闪退 (0xc0000409) | ① SignalBridge 缺失 → DSP 链不绑定 ② bass_fx.dll 缺失 ③ FFT `BASS_DATA_FFT_INDIVIDUAL` 对立体声返回 1024 floats 但缓冲区只 512 → 栈溢出 | ① 加桥接 lambda ② 下载 bass_fx.dll ③ 移除 INDIVIDUAL flag |
| 2026-06-16 | FLAC 被标记为"需要额外 DLL" | 过时信息 | 实测确认 bass.dll 内置 FLAC 支持 |

### 待观察

- AAC / OGG 格式未测试，可能需额外 BASS 插件 DLL
- 日语文件名路径已通过 `QString::fromLocal8Bit` + `BASS_UNICODE` 处理，目前正常

---

## 构建与测试

### 构建

```powershell
cd C:\Users\liwufan\Desktop\WaveRider\build
cmake .. -G "Visual Studio 17 2022" -A x64 -DQt5_DIR="C:/ProgramData/anaconda3/Library/lib/cmake/Qt5"
cmake --build . --config Release
```

### 自动化测试

```powershell
# 命令行播放测试
.\src\Release\WaveRider.exe "path\to\audio.mp3"

# 测试脚本：启动 → 等 N 秒 → 检查是否崩溃
$proc = Start-Process -FilePath $exe -ArgumentList "`"$file`"" -PassThru
Start-Sleep -Seconds 5
if (-not $proc.HasExited) { "OK" } else { "CRASH" }
```

### 测试文件

桌面 `WR-test/` 文件夹：
- `世界一可愛い私 - 藤田ことね.mp3` (9.2 MB) ✅
- `藤田琴音闹铃一分钟.wav` (16.8 MB) ✅
- `鹿乃 - Calc..flac` (21.7 MB) ✅

---

## 发布历史

| 版本 | 日期 | 内容 |
|------|------|------|
| v1.0 | 2026-06-16 | 初始发布，全部核心功能 |
| v1.1 | 2026-06-16 | 修复 MP3 播放崩溃（SignalBridge + bass_fx.dll + FFT overflow） |

下载: https://github.com/liwufan-xizi/WaveRider/releases

---

## 目录结构速查

```
src/
├── core/        SignalBus, ConfigManager, Constants
├── audio/       AudioEngine (BASS封装), AudioMetadata
├── playlist/    PlaylistModel, PlaylistManager, PlayModeEngine
├── dsp/         IDSPEffect, DSPChain, DSPPluginLoader, builtin/
├── skin/        SkinManager, BackgroundManager, ThemeConfig, ColorExtractor
├── lyrics/      LyricParser, LyricFetcher, LyricModel
├── favorites/   FavoritesManager
└── ui/          MainWindow, PlayerControlBar, TrackInfoWidget, SpectrumWidget,
                 PlaylistPanel/Delegate, LyricPanel, DSPPanel, FavoritesPanel,
                 EqualizerWidget, SkinSelectorDialog, BackgroundDialog,
                 VisualArea, widgets/
```

---

*此文件由 Claude 维护，每完成一项开发任务后自动更新。README 由项目作者自行编写。*

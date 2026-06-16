# WaveRider — 本地音频播放器

## 项目概述
Windows 本地音频播放器，Qt 5.15.2 (C++17) + BASS 2.4 音频引擎，Phigros 风格的完全自定义 UI。

## 核心需求
- 仅音频（MP3/FLAC/WAV/AAC/OGG），不含视频
- 皮肤/主题系统（QSS）+ 用户自定义背景图片（本地图片，5种显示模式）
- 背景自适应配色：换背景图 → 自动提取主色调 → 所有控件颜色跟随变化
- DSP 音效插件系统（均衡器/混响/压缩），支持外部 DLL 插件
- 功能：核心播放 + 播放列表(M3U) + 歌词(LRC + 在线爬虫) + 4种播放模式 + 收藏 + 频谱
- 高可拓展性：皮肤可扩展、DSP 插件可扩展、布局预留动态缩放扩展点

## 技术栈
- Qt 5.15.2（Conda MSVC 编译版，路径 `C:\ProgramData\anaconda3\Library`）
- BASS 2.4 音频库（`vendor/bass/`，x64）
- Qt5::Network（歌词在线爬虫）
- MSVC 2022 Build Tools 编译器
- CMake 4.3.2
- 平台：Windows 11 x64

## 架构
```
src/
├── core/        SignalBus (全局信号解耦), ConfigManager, Constants
├── audio/       AudioEngine (BASS封装), AudioMetadata, BASSInitGuard
├── playlist/    PlaylistModel (QAbstractListModel), PlaylistManager, PlayModeEngine
├── dsp/         IDSPEffect (抽象基类), DSPChain (效果链), DSPPluginLoader (插件加载),
│                builtin/ (EqualizerEffect, CompressorEffect, ReverbEffect)
├── skin/        SkinManager (QSS主题), BackgroundManager (背景+触发配色提取),
│                ThemeConfig (动态配色单例), ColorExtractor (HSV主色提取), StyleSheetBuilder (STUB)
├── lyrics/      LyricParser (LRC解析), LyricFetcher (在线爬虫+缓存), LyricModel (数据模型)
├── favorites/   FavoritesManager (JSON持久化)
└── ui/          MainWindow (16:9 Phigros布局), VisualArea (频谱/歌词切换),
                 PlayerControlBar (全自定义绘制: 菱形按钮+细进度条+音量条+心形+汉堡),
                 TrackInfoWidget (自定义绘制), PlaylistPanel (320px滑动覆盖面板+PlaylistDelegate),
                 LyricPanel (自定义绘制滚动歌词), SpectrumWidget (FFT频谱+峰值+发光),
                 DSPPanel (480px浮动面板), FavoritesPanel (320px滑动覆盖面板),
                 EqualizerWidget (10段EQ自定义滑块), PlaylistDelegate (48px Phigros风格行),
                 SkinSelectorDialog (STUB), BackgroundDialog (STUB),
                 widgets/ (VolumeKnob自定义, SeekSlider-STUB, AnimatedButton-STUB)
```

## SignalBus 解耦模式
所有跨模块通信通过 `SignalBus::instance()` 单例信号总线：
```
AudioEngine ──emit──→ SignalBus ──listen──→ UI面板
PlaylistManager ──emit──→ SignalBus ──listen──→ MainWindow
BackgroundManager ──emit──→ SignalBus ──listen──→ MainWindow
```
规则：core 不依赖任何模块；ui 可依赖所有模块；其他模块仅依赖 core。

信号清单：
- 播放: `trackStarted`, `trackFinished`, `playbackPaused/Resumed/Stopped`, `positionChanged`, `volumeChanged`, `playbackStateChanged`
- 列表: `playlistChanged`, `currentTrackChanged`, `playModeChanged`
- 主题: `skinChanged`, `themeColorsChanged`, `backgroundChanged`
- DSP: `effectAdded/Removed`, `effectBypassChanged`, `dspChainChanged` (预留)
- 收藏: `favoriteAdded/Removed`
- 歌词: `lyricLineChanged`

## UI 布局 (16:9 Phigros 风格)

```
┌──────────────────────────────────────────────────────┐
│  ≡  WaveRider                            ─  × │  菜单栏 (透明)
├──────────────────────────────────────────────────────┤
│  ♪  Track Title                      3:42            │  TrackInfoWidget 56px
│     Artist · Album · 44kHz · 320kbps                 │  自定义绘制
├──────────────────────────────────────────────────────┤
│            ╭────────────────────────╮                │
│            │     VisualArea         │                │  中央视觉区 (stretch)
│            │  频谱/歌词 (点击切换)    │                │  QStackedWidget
│            ╰────────────────────────╯                │
├──────────────────────────────────────────────────────┤
│  ●━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━● 1:23    │  细进度条 + 菱形按钮
│       ⏮      ◆      ⏭       ↻1    🔊 ──  ♡ ☰    │  PlayerControlBar 80px
└──────────────────────────────────────────────────────┘
```
- 预设分辨率: 800×450 / 960×540 / 1280×720（View → Window Size 菜单切换）
- 播放列表: ☰ 按钮 → 320px 面板从右侧滑入(250ms OutCubic)，半透明遮罩点击关闭
- 收藏面板: ♡ 按钮 / Ctrl+F → 同款滑动面板，Delete 移除收藏
- DSP 面板: Ctrl+D → 480px 浮动面板居中覆盖
- 窗口背景: BackgroundManager 渲染，paintEvent 中绘制在最底层

## 主题与配色系统

### SkinManager (QSS 主题)
- 扫描 `skins/` 目录下的子文件夹，加载 `theme.qss`
- 解析 `/* @vars $name: value; */` 块进行变量替换
- 通过 `qApp->setStyleSheet()` 应用
- 已内置主题: `DarkModern`（旧）、`Phigros`（新）

### ThemeConfig 单例（动态配色）
- 存储当前 UI 配色：`accent`, `accentDim`, `textPrimary`, `textSecondary`, `surface`, `surfaceBorder`, `overlayAlpha`, `isDarkBg`
- 所有自定义绘制控件（PlayerControlBar, TrackInfoWidget, LyricPanel, SpectrumWidget）从此读取颜色
- `themeColorsChanged()` 信号 → 控件 repaint

### ColorExtractor（背景主色提取）
- `extract(QPixmap, gridSize=12)` → `ColorExtractionResult{ dominantAccent, averageValue, isDark }`
- 算法: HSV 色相直方图 + 饱和度/亮度 clamp
- 由 BackgroundManager::setBackground() 自动触发

### 配色数据流
```
用户换背景 → BackgroundManager::setBackground()
  → ColorExtractor::extract()
  → 构建 ThemeColors → ThemeConfig::setColors()
  → emit themeColorsChanged()
  → PlayerControlBar/TrackInfo/LyricPanel/Spectrum 全部 repaint
```
无背景时 → `ThemeConfig::resetToDefaults()` → Phigros 薄荷色 `#00d4aa`

### 皮肤目录
- `skins/DarkModern/theme.qss` — 旧版深蓝主题（标准 Qt 控件样式）
- `skins/Phigros/theme.qss` — 新版极简皮肤（菜单、滚动条、列表、输入框、状态栏）

## 音频引擎 (AudioEngine)

BASS 2.4 封装，核心接口：
- `initialize(HWND)` / `shutdown()` — 生命周期
- `loadFile(path)` / `unload()` — 加载/卸载音频文件
- `play()` / `pause()` / `stop()` / `togglePlayPause()`
- `seek(seconds)` / `positionMs()` / `durationMs()`
- `setVolume(0.0-1.0)` / `volume()`
- `streamHandle()` — 暴露 `HSTREAM` 给 DSP 链和频谱 FFT
- `state()` → `PlaybackState` 枚举 (Stopped/Playing/Paused)

内部定时器: `Config::PosTimerMs = 100ms` 轮询位置 → `positionUpdated(posMs, durMs)`

## 播放列表 (PlaylistModel / PlaylistManager / PlayModeEngine)

- **PlaylistModel**: `QAbstractListModel`，存储 `AudioMetadata` 列表，支持拖放排序
  - 角色: TitleRole, ArtistRole, AlbumRole, DurationTextRole, IsPlayingRole, IsFavoriteRole
- **PlaylistManager**: M3U 加载/保存、文件/文件夹打开对话框、播放导航 (playAt/next/prev)
- **PlayModeEngine**: 4种播放模式 — Sequential / LoopAll / LoopOne / Shuffle
- 播放列表通过 ☰ 按钮切换 → 320px 面板从右侧滑入

### PlaylistDelegate (自定义行绘制)
- QStyledItemDelegate 子类，每行 48px 高
- 双行布局: 标题 11px DemiBold / 艺术家 · 时长 9px Light
- 当前播放行: 左侧 3px accent 色竖条指示器
- 收藏行: 右侧 accent 心形图标
- 状态: 交替行微斑马纹 / hover surface 高亮 / selected accent 背景 + 标题变色
- 底部分割线 0.5px alpha 40，全部颜色从 ThemeConfig 读取

## 歌词系统

### 三级加载策略
```
曲目开始 → onTrackStarted()
  1. 本地同目录 .lrc 文件 (LyricParser::findLrcFile)
  2. 本地缓存 (LyricFetcher::cachedLyrics, AppData/lyrics/)
  3. 在线爬取 (LyricFetcher::fetchLyrics → lrclib.net API)
```

### LyricParser
- LRC 格式解析: `[mm:ss.xx]text`，支持多时间戳行、元标签 `[ti:]` / `[ar:]`
- 编码回退: UTF-8 → local8bit
- 静态方法: `parse(content)`, `parseFile(path)`, `findLrcFile(audioPath)`

### LyricFetcher（在线爬虫）
- 数据源: `lrclib.net/api/search?artist_name=...&track_name=...`
- 异步 `QNetworkAccessManager` + 10s 超时
- 本地缓存: `QStandardPaths::AppDataLocation/lyrics/{key}.lrc`
- 信号: `lyricsFetched`, `lyricsNotFound`, `fetchError`

### LyricModel
- 存储解析后的 `LyricData`
- `lineIndexAtTime(positionMs)` — 二分查找当前行
- `loadFromFile()` / `loadFromText()` → emit `modelLoaded()`

### LyricPanel（自定义绘制）
- 垂直滚动歌词，当前行行间距 44px
- ThemeConfig 配色: 当前行 accent 色 + 左侧 3px 竖条指示器
- 字体: "Segoe UI Light"，行间线性插值平滑滚动
- 状态: Idle / Searching / NotFound / Loaded

## 频谱可视化 (SpectrumWidget)

- FFT 数据: `BASS_ChannelGetData(BASS_DATA_FFT1024 | INDIVIDUAL | REMOVEDC)` → 512 bins
- 对数映射: 512 bins → 64 bars（低频高分辨率）
- 平滑: 非对称 EMA（attack 瞬时, decay 0.35）
- 峰值落点: 缓慢下落的小圆点
- 发光: 条背后径向渐变
- 配色: 单色 accent 渐变（底部 accent → 顶部 lighter(160)）
- 定时器: `Config::FftTimerMs = 50ms`，仅 show 时运行

## 收藏系统

### FavoritesManager
- JSON 文件持久化: `ConfigManager::appDataDir()/favorites.json`
- 接口: `add(path)`, `remove(path)`, `toggle(path)`, `isFavorite(path)`, `allFavorites()`
- 单例模式，SignalBus 转发 `favoriteAdded` / `favoriteRemoved`

### FavoritesPanel (覆盖面板)
- 320px 右侧滑动面板 (QPropertyAnimation 250ms OutCubic)，半透明背景遮罩点击关闭
- QStandardItemModel 数据模型，`AudioMetadata::fromFile()` 读取元数据
- 搜索过滤 (标题/艺术家)，Delete 键移除收藏
- 双击曲目 → 播放（若不在播放列表中则先添加）
- Ctrl+F / View → Favorites 切换，SignalBus 自动刷新

### PlayerControlBar 心形按钮
- 心形 QPainterPath (cubic Bezier)，位于汉堡按钮左侧
- 收藏时 accent 填充 ♥，未收藏时轮廓 ♡（hover 时 accent 色）
- `favoriteToggleClicked()` 信号 → MainWindow → FavoritesManager::toggleFavorite()

## DSP 系统

### 架构
```
IDSPEffect (QObject抽象基类, signals: bypassChanged, parametersChanged)
  ├── EqualizerEffect  — 10段参量EQ (BASS_DX8_PARAMEQ × 10)
  ├── CompressorEffect — 动态压缩    (BASS_DX8_COMPRESSOR)
  └── ReverbEffect     — 混响        (BASS_DX8_REVERB)
DSPChain — 效果链管理器，绑定 HSTREAM，支持 add/remove/reapply/masterBypass
DSPPluginLoader — 外部插件加载 (框架已就位，扫描/实例化逻辑待实现)
```

### 核心 API
- `IDSPEffect::applyToStream(HSTREAM)` / `removeFromStream(HSTREAM)` — 管理 BASS FX handles
- `IDSPEffect::setBypass(bool)` — 旁通开关，emit bypassChanged
- `DSPChain::addEffect()` / `removeEffect()` / `findEffect(id)`
- `DSPChain::setStream()` + `reapplyAll()` — 切歌时重挂效果
- `DSPChain::setMasterBypass()` — 全局旁通

### EqualizerEffect (10段参量EQ)
- 频率: 32, 64, 125, 250, 500, 1K, 2K, 4K, 8K, 16K Hz
- 增益: ±15 dB，带宽固定 1.0 八度 (12 半音)
- 预设: Flat / Rock / Pop / Jazz / Classical / Vocal / Bass Boost / Treble Boost
- Q_PROPERTY per band → QSS / 属性绑定可用

### 数据流
```
AudioEngine::loadFile → new HSTREAM
MainWindow::onTrackStarted → DSPChain::setStream(newStream)
                           → DSPChain::reapplyAll()
                           → 逐个 IDSPEffect::applyToStream() → BASS_ChannelSetFX()
DSPPanel UI → EqualizerWidget::bandChanged → EqualizerEffect::setBandGain()
            → BASS_FXSetParameters() 实时更新
```

### 关键发现
- BASS_DX8 效果（PARAMEQ/COMPRESSOR/REVERB）**内置在 bass.dll**，无需 bass_fx.dll
- bass_fx.dll 只提供额外的 BFX 效果（非必需），DX8 效果开箱即用

### EqualizerWidget (自定义绘制)
- 10条垂直滑块，Phigros 风格：细线轨道(3px) + 圆形手柄(5px radius)
- Accent 色填充(0dB→当前增益)，hover 发光圈
- 鼠标交互: click-to-set, drag-to-adjust, wheel ±0.5dB
- 从 ThemeConfig 读取颜色

### DSPPanel (覆盖面板)
- 480px 宽浮动面板，自定义标题栏 + 圆形主开关 + 预设按钮行
- Phigros 半透明 surface 背景 + accent 色主题
- Ctrl+D 或 View → DSP Effects 切换显示
- 居中覆盖在窗口上

## 构建命令
```powershell
# 配置（仅首次或 CMakeLists 变更时）
cd C:\Users\liwufan\Desktop\WaveRider\build
cmake .. -G "Visual Studio 17 2022" -A x64 -DQt5_DIR="C:/ProgramData/anaconda3/Library/lib/cmake/Qt5"

# 编译
cmake --build . --config Release

# 运行
C:\Users\liwufan\Desktop\WaveRider\build\src\Release\WaveRider.exe
```

## 开发状态 (2026-06-16)

### ✅ 完成
| 模块 | 内容 |
|------|------|
| 音频引擎 | BASS 封装、元数据读取(ID3v1)、位置轮询 |
| 播放列表 | PlaylistModel + M3U + 4种播放模式 |
| 歌词系统 | LRC解析 + 在线爬虫(lrclib.net) + 本地缓存 + 滚动显示 |
| 频谱可视化 | FFT1024 + 64条对数映射 + 峰值落点 + 发光效果 |
| 主题系统 | SkinManager(QSS) + ThemeConfig(动态配色) + ColorExtractor(主色提取) |
| 背景系统 | 5种显示模式 + 暗色蒙层 + 背景触发自动配色 |
| 收藏 | FavoritesManager JSON持久化 + FavoritesPanel (滑动覆盖面板 + heart按钮 + Ctrl+F) |
| UI 布局 | 16:9 Phigros 风格 + 3档预设分辨率 |
| UI 控件 | PlayerControlBar 全自定义(菱形按钮、细进度条、音量条、心形收藏♡、汉堡☰) |
| | TrackInfoWidget 自定义(封面占位符、单行信息流) |
| | LyricPanel 自定义(ThemeConfig配色、左侧竖条指示器) |
| | SpectrumWidget 自定义(单色渐变、峰值、发光) |
| | PlaylistDelegate 自定义48px行(双行布局、播放指示条、收藏心形) |
| | VisualArea 频谱/歌词点击切换 |
| 皮肤 | DarkModern(旧) + Phigros(新) 两套 QSS |
| DSP 系统 | IDSPEffect + DSPChain + 3个内置效果 + EqualizerWidget + DSPPanel |
| | 10段参量EQ (32-16KHz ±15dB) + 8预设 + 压缩器 + 混响 |
| PlaylistPanel | 右侧320px滑入/滑出动画 + 背景遮罩 + 搜索过滤 + PlaylistDelegate自定义48px行 |

### 🔄 待做
| 模块 | 说明 |
|------|------|
| SkinSelector / BackgroundDialog | 对话框待实现 |
| 内置主题 | 后期协商 |
| 动态缩放 | 扩展点已预留，resizeEvent hook 待实现 |
| DSP 插件加载 | DSPPluginLoader 扫描/实例化逻辑待实现 |

## 待下载（可选功能）
将以下文件放入 `vendor/bass/` 解锁 FLAC/AAC：
- bassflac.dll + bassflac.lib (FLAC)
- bass_aac.dll + bass_aac.lib (AAC)
来源: http://www.un4seen.com/
（DSP 均衡器/压缩/混响已通过 bass.dll 内置 DX8 效果实现，无需额外下载）

## BASS DLL 运行时
bass.dll 通过 CMake POST_BUILD 自动复制到 exe 同目录。
Qt DLL 从 `C:\ProgramData\anaconda3\Library\bin` 加载（需在 PATH 中）。

# WaveRider

一款 Windows 本地音频播放器，基于 Qt 5 与 BASS 音频引擎构建。采用 Phigros 极简美学设计，全自定义绘制控件，支持背景自适应配色。

![平台](https://img.shields.io/badge/平台-Windows%20x64-blue)
![语言](https://img.shields.io/badge/语言-C%2B%2B17-00599C)
![界面](https://img.shields.io/badge/界面-Qt%205.15.2-green)
![音频](https://img.shields.io/badge/音频-BASS%202.4-orange)

## 功能特性

### 音频播放
- 支持 **MP3、FLAC、WAV、AAC、OGG**（AAC/FLAC 需可选 BASS 附加 DLL）
- BASS 2.4 引擎，无缝播放，低延迟跳转
- 音量控制，支持静音切换

### 播放列表
- **M3U** 播放列表导入/导出
- 拖拽排序曲目
- 文件夹导入（递归扫描）
- 4 种播放模式：**顺序播放**、**列表循环**、**单曲循环**、**随机播放**
- 列表内搜索/过滤

### 视觉体验
- **Phigros 极简风格** — 菱形播放按钮、细线进度条、几何图标
- **自定义背景图片**，5 种显示模式：填充、适配、拉伸、平铺、居中
- **自适应配色** — 自动提取背景主色调，全局控件颜色跟随变化
- **皮肤系统**，QSS 样式表主题（内置 DarkModern + Phigros 两款）
- 3 档预设分辨率：800×450 / 960×540 / 1280×720（支持自由缩放 + 动态适配）

### FFT 频谱可视化
- 64 条对数频率显示（FFT1024）
- 峰值保持 + 缓慢下落光点
- 条后径向发光效果
- 单色 accent 渐变（跟随主题色）

### 歌词
- **LRC** 格式解析，支持多时间戳
- 三级自动发现：同目录 `.lrc` → 本地缓存 → 在线获取（lrclib.net）
- 垂直平滑滚动，当前行高亮 + 左侧指示条
- 编码回退（UTF-8 → local8bit）

### 收藏
- 心形按钮一键收藏
- JSON 持久化存储（`AppData/favorites.json`）
- 右侧滑入面板，支持搜索和批量管理
- `Ctrl+F` 快捷键

### DSP 音效（内置）
- **10 段参量均衡器**（32 Hz – 16 kHz，±15 dB），8 种预设
- **压缩器**（阈值、压缩比、起音、释放、增益）
- **混响**（房间大小、阻尼、干湿比）
- 总旁通开关
- 外部 DSP DLL 插件系统

### 快捷键
| 快捷键 | 功能 |
|--------|------|
| `Ctrl+O` | 打开文件 |
| `Ctrl+D` | 切换 DSP 面板 |
| `Ctrl+F` | 切换收藏面板 |
| `Ctrl+Q` | 退出 |

## 界面预览

```
┌──────────────────────────────────────────────────────┐
│  ≡  WaveRider                            ─  × │  菜单栏
├──────────────────────────────────────────────────────┤
│  ♪  曲目标题                        3:42             │  曲目信息 (56px)
│     艺术家 · 专辑 · 44kHz · 320kbps                  │
├──────────────────────────────────────────────────────┤
│            ╭────────────────────────╮                 │
│            │     视觉区域            │                 │  频谱 / 歌词
│            │  频谱或歌词（点击切换）   │                 │
│            ╰────────────────────────╯                 │
├──────────────────────────────────────────────────────┤
│  ●━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━● 1:23     │  进度条
│       ⏮      ◆      ⏭       ↻1    🔊 ──  ♡ ☰     │  控制栏 (80px)
└──────────────────────────────────────────────────────┘
```

## 环境要求

### 运行环境
- **Windows 10/11 x64**
- [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)（MSVC 2022）

### 开发环境
- **Qt 5.15.2**（MSVC 编译版）
- **MSVC 2022 Build Tools** 或 Visual Studio 2022
- **CMake 3.20+**
- **BASS 2.4** 音频库（已包含在 `vendor/bass/` 中）

## 快速开始

### 下载运行

1. 从 [Releases](../../releases) 下载最新版本
2. 解压到任意目录
3. 运行 `WaveRider.exe`

> `bass.dll` 和 `skins/` 文件夹必须与可执行文件在同一目录。

### 从源码构建

```powershell
# 1. 克隆仓库
git clone https://github.com/liwufan-xizi/WaveRider.git
cd WaveRider

# 2. 配置 CMake
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DQt5_DIR="C:/ProgramData/anaconda3/Library/lib/cmake/Qt5"

# 3. 编译
cmake --build . --config Release

# 4. 运行
.\src\Release\WaveRider.exe
```

> **注意：** 请根据你的 Qt 安装路径调整 `Qt5_DIR`。Conda 用户通常为 `C:/ProgramData/anaconda3/Library/lib/cmake/Qt5`。

### 可选：解锁 FLAC/AAC

从 [un4seen.com](http://www.un4seen.com/) 下载以下文件放入 `vendor/bass/`：
- `bassflac.dll` + `bassflac.lib`（FLAC）
- `bass_aac.dll` + `bass_aac.lib`（AAC）

编译时会自动复制到输出目录。

## 项目结构

```
WaveRider/
├── README.md                   # 英文说明
├── README_zh.md                # 中文说明
├── CLAUDE.md                   # 开发者文档
├── CMakeLists.txt              # 根 CMake 配置
├── src/
│   ├── CMakeLists.txt          # 源码构建配置
│   ├── main.cpp                # 入口
│   ├── core/                   # SignalBus 信号总线、ConfigManager 配置管理、常量
│   ├── audio/                  # AudioEngine (BASS 封装)、AudioMetadata 音频元数据
│   ├── playlist/               # PlaylistModel 播放列表、PlaylistManager、PlayModeEngine
│   ├── dsp/                    # DSPChain 效果链、DSPPluginLoader 插件加载、内置效果
│   ├── skin/                   # SkinManager 皮肤、BackgroundManager 背景、ThemeConfig 配色、ColorExtractor
│   ├── lyrics/                 # LyricParser 歌词解析、LyricFetcher 在线获取、LyricModel
│   ├── favorites/              # FavoritesManager 收藏管理 (JSON 持久化)
│   └── ui/                     # MainWindow 主窗口、全部自定义控件、对话框、面板
├── skins/
│   ├── Phigros/theme.qss       # 默认皮肤 — Phigros 极简
│   └── DarkModern/theme.qss    # 备选深色主题
├── vendor/
│   └── bass/                   # BASS 2.4 头文件 + DLL
└── resources/
    └── app.qrc                 # Qt 资源文件
```

## 架构设计

所有模块通过 **SignalBus** 单例信号总线解耦通信：

```
AudioEngine ──→ SignalBus ──→ UI 面板
PlaylistMgr ──→ SignalBus ──→ MainWindow
BgManager   ──→ SignalBus ──→ MainWindow
```

**依赖规则：** `core/` 不依赖任何模块；`ui/` 可依赖所有模块；其他模块仅依赖 `core/`。

## 皮肤制作

在 `skins/` 下新建文件夹即可创建自定义皮肤：

```
skins/我的主题/
└── theme.qss     # QSS 样式表（可选 @vars 变量块）
```

`@vars` 注释块定义颜色变量，加载时自动替换到样式表中：

```css
/* @vars
$primary: #00d4aa;
$bg_main: #0a0a14;
$text_primary: #e8e8e8;
...
*/
```

## DSP 插件开发

外部 DSP 效果以 DLL 形式加载。创建插件：

```
plugins/
├── 我的效果.dll       # 导出 `waveRiderDspFactory`
└── 我的效果.json      # { "id": "my_effect", "displayName": "我的效果", ... }
```

DLL 必须导出工厂函数：

```cpp
extern "C" __declspec(dllexport) WaveRider::IDSPEffect* waveRiderDspFactory();
```

JSON 清单格式：

```json
{
    "id": "my_effect",
    "displayName": "我的效果",
    "version": "1.0.0",
    "author": "作者名",
    "description": "效果描述"
}
```

## 许可证

MIT License — 详见 [LICENSE](LICENSE)。

## 致谢

- **BASS Audio Library** — [un4seen.com](http://www.un4seen.com/)
- **lrclib.net** — 歌词数据源
- **Phigros** — UI 设计灵感（Pigeon Games）
- **Stellio Player** — 布局设计参考

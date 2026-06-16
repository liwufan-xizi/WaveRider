# WaveRider

A customizable local audio player for Windows, built with Qt 5 and BASS audio engine. Features a Phigros-inspired minimalist UI with fully custom-painted controls and adaptive color theming.

![Platform](https://img.shields.io/badge/platform-Windows%20x64-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C)
![UI](https://img.shields.io/badge/UI-Qt%205.15.2-green)
![Audio](https://img.shields.io/badge/audio-BASS%202.4-orange)

## Features

### Audio Playback
- Supports **MP3, FLAC, WAV, AAC, OGG** (AAC/FLAC require optional BASS add-on DLLs)
- BASS 2.4 engine with gapless playback and low-latency seeking
- Volume control with mute toggle

### Playlist Management
- **M3U** playlist import/export
- Drag-and-drop file reordering
- Folder import (recursive scan)
- 4 playback modes: **Sequential**, **Loop All**, **Loop One**, **Shuffle**
- Search/filter within playlist

### Visual Experience
- **Phigros-inspired minimalist design** — diamond play button, thin seek bar, geometric icons
- **Background image** with 5 display modes: Fill, Fit, Stretch, Tile, Center
- **Adaptive color theming** — automatically extracts dominant color from your background and tints all UI elements
- **Skin system** with QSS stylesheet themes (DarkModern + Phigros included)
- 3 preset resolutions: 800×450 / 960×540 / 1280×720 (free resize with dynamic scaling)

### FFT Spectrum Visualization
- 64-bar logarithmic frequency display (FFT1024)
- Peak hold with falling dots
- Radial glow behind active bars
- Single-color accent gradient (matches your theme)

### Lyrics
- **LRC** format parsing with multi-timestamp support
- Auto-discovery: same-directory `.lrc` → local cache → online fetch (lrclib.net)
- Smooth vertical scrolling with current-line accent indicator
- Encoding fallback (UTF-8 → local8bit)

### Favorites
- Heart-shaped toggle button
- Persistent JSON storage (`AppData/favorites.json`)
- Slide-in panel with search and batch management
- `Ctrl+F` shortcut

### DSP Effects (Built-in)
- **10-band Parametric EQ** (32 Hz – 16 kHz, ±15 dB) with 8 presets
- **Compressor** (threshold, ratio, attack, release, gain)
- **Dynamic Reverb** (room size, damping, wet/dry mix)
- Master bypass toggle
- Plugin system for external DSP DLLs

### Keyboard Shortcuts
| Shortcut | Action |
|----------|--------|
| `Ctrl+O` | Open files |
| `Ctrl+D` | Toggle DSP panel |
| `Ctrl+F` | Toggle favorites panel |
| `Ctrl+Q` | Quit |

## Screenshots

```
┌──────────────────────────────────────────────────────┐
│  ≡  WaveRider                            ─  × │  Menu bar
├──────────────────────────────────────────────────────┤
│  ♪  Track Title                      3:42             │  Track info (56px)
│     Artist · Album · 44kHz · 320kbps                  │
├──────────────────────────────────────────────────────┤
│            ╭────────────────────────╮                 │
│            │     VisualArea         │                 │  Spectrum / Lyrics
│            │  Spectrum or Lyrics    │                 │  (click to switch)
│            ╰────────────────────────╯                 │
├──────────────────────────────────────────────────────┤
│  ●━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━● 1:23     │  Seek bar
│       ⏮      ◆      ⏭       ↻1    🔊 ──  ♡ ☰     │  Controls (80px)
└──────────────────────────────────────────────────────┘
```

## Requirements

### System
- **Windows 10/11 x64**
- [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) (MSVC 2022)

### Development
- **Qt 5.15.2** (MSVC build)
- **MSVC 2022 Build Tools** or Visual Studio 2022
- **CMake 3.20+**
- **BASS 2.4** audio library (included in `vendor/bass/`)

## Quick Start

### Download & Run

1. Download the latest release from [Releases](../../releases)
2. Extract the archive
3. Run `WaveRider.exe`

The `bass.dll` and `skins/` folder must be in the same directory as the executable.

### Build from Source

```powershell
# 1. Clone
git clone https://github.com/YOUR_USERNAME/WaveRider.git
cd WaveRider

# 2. Configure
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DQt5_DIR="C:/ProgramData/anaconda3/Library/lib/cmake/Qt5"

# 3. Build
cmake --build . --config Release

# 4. Run
.\src\Release\WaveRider.exe
```

> **Note:** Adjust `Qt5_DIR` to your Qt installation path. For Conda users, it's typically `C:/ProgramData/anaconda3/Library/lib/cmake/Qt5`.

### Optional: FLAC/AAC Support

Download these DLLs from [un4seen.com](http://www.un4seen.com/) and place them in `vendor/bass/`:
- `bassflac.dll` + `bassflac.lib`
- `bass_aac.dll` + `bass_aac.lib`

They will be automatically copied to the output directory during build.

## Project Structure

```
WaveRider/
├── README.md
├── CLAUDE.md                   # Developer documentation
├── CMakeLists.txt              # Root CMake
├── src/
│   ├── CMakeLists.txt          # Source build config
│   ├── main.cpp                # Entry point
│   ├── core/                   # SignalBus, ConfigManager, Constants
│   ├── audio/                  # AudioEngine (BASS wrapper), AudioMetadata
│   ├── playlist/               # PlaylistModel, PlaylistManager, PlayModeEngine
│   ├── dsp/                    # DSPChain, DSPPluginLoader, builtin effects
│   ├── skin/                   # SkinManager, BackgroundManager, ThemeConfig, ColorExtractor
│   ├── lyrics/                 # LyricParser, LyricFetcher, LyricModel
│   ├── favorites/              # FavoritesManager (JSON persistence)
│   └── ui/                     # MainWindow, all custom widgets, dialogs, panels
├── skins/
│   ├── Phigros/theme.qss       # Default skin — Phigros minimalist
│   └── DarkModern/theme.qss    # Alternative dark theme
├── vendor/
│   └── bass/                   # BASS 2.4 headers + DLL
└── resources/
    └── app.qrc                 # Qt resource file
```

## Architecture

Modules communicate through a **SignalBus** singleton — a decoupled signal/slot hub:

```
AudioEngine ──→ SignalBus ──→ UI panels
PlaylistMgr ──→ SignalBus ──→ MainWindow
BgManager   ──→ SignalBus ──→ MainWindow
```

**Rules:** `core/` depends on nothing. `ui/` may depend on all modules. Other modules only depend on `core/`.

## Skins

Create your own skin by adding a folder to `skins/`:

```
skins/MyTheme/
└── theme.qss     # QSS stylesheet with optional @vars block
```

The `@vars` comment block defines color variables that are substituted into the stylesheet:

```css
/* @vars
$primary: #00d4aa;
$bg_main: #0a0a14;
$text_primary: #e8e8e8;
...
*/
```

## DSP Plugins

External DSP effects can be loaded as DLLs. Create a plugin:

```
plugins/
├── MyEffect.dll      # exports `waveRiderDspFactory`
└── MyEffect.json     # { "id": "my_effect", "displayName": "My Effect", ... }
```

The DLL must export: `extern "C" __declspec(dllexport) IDSPEffect* waveRiderDspFactory();`

## License

MIT License — see [LICENSE](LICENSE) for details.

## Credits

- **BASS Audio Library** — [un4seen.com](http://www.un4seen.com/)
- **lrclib.net** — Lyrics data source
- **Phigros** — UI design inspiration (Pigeon Games)
- **Stellio Player** — Layout design inspiration

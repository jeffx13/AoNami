# <div align="center"><img src="resources/icon_wave.png" alt="AoNami" width="144" height="144"/><br/>AoNami</div>

<div align="center">

Powerful Qt/QML video player and streaming explorer powered by libmpv.

[![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)](https://www.microsoft.com/windows)
[![Qt](https://img.shields.io/badge/Qt-41CD52?style=for-the-badge&logo=qt&logoColor=white)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![mpv](https://img.shields.io/badge/libmpv-000000?style=for-the-badge&logo=mpv&logoColor=white)](https://mpv.io/)

</div>

## ✨ Features

- **Multi-source playback**: Local files and online streaming
- **Providers**: HiAnime, IYF, AnimePahe, AllAnime, Bilibili, QQVideo, SeedBox
- **Playlists & library**: Watch history, continue watching, episode tracking
- **Downloads**: M3U8 support via `N_m3u8DL-RE`, muxing via `ffmpeg`
- **Player controls**: Speed, PiP, subtitles, precise volume, resume position
- **Modern UI**: Responsive Qt Quick UI, keyboard-centric navigation

## 📦 Binaries

- Windows 10/11 builds: extract and run `AoNami.exe`.
- The runtime includes helper tools (`ffmpeg.exe`, `yt-dlp.exe`, `N_m3u8DL-RE.exe`).

## 🧪 Quick Start

- Drag & drop files/folders to play
- Press `E` to open a file, `Ctrl+E` to open a folder
- Use the Explorer page to search and play online content
- Use the Library page to manage and resume shows

## 🎮 Key Shortcuts

- **Play/Pause**: Space
- **Fullscreen**: F (Esc to exit)
- **PiP**: Ctrl+A
- **Seek**: ←/→ (±5s), Ctrl+Z / End (−90s), Ctrl+X / PgDown (+90s)
- **Speed**: `+` / `-` (±0.1x), `R` toggle 1.0x/2.0x, hold Shift for 2x
- **Volume**: ↑/Q, ↓/A
- **Playlist**: P/W toggle, Ctrl+S / Ctrl+D prev/next item, Ctrl+Shift+S / Ctrl+Shift+D prev/next playlist
- **Navigation**: 1..6 to switch pages, Ctrl+Tab / Ctrl+Shift+Tab cycle, Alt+←/→ history

## 🛠️ Build from Source (Windows / MSYS2 MinGW64)

Prerequisites:

- Windows 10/11, CMake ≥ 3.16
- Qt 6 (Quick, QuickControls2, Qml, Concurrent, Core5Compat, Sql, Network)
- CryptoPP, libxml2, libmpv

Install toolchain (MSYS2 MinGW64 shell):

```bash
pacman -S --needed git cmake ninja
pacman -S --needed mingw-w64-x86_64-toolchain
pacman -S --needed mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-declarative mingw-w64-x86_64-qt6-svg
pacman -S --needed mingw-w64-x86_64-cryptopp mingw-w64-x86_64-libxml2
```

Configure and build:

```bash
git clone https://github.com/jeffx13/AoNami.git
cd AoNami
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/msys64/mingw64"
cmake --build build --config Release
cmake --install build
```

Notes:

- CMake deploys Qt QML runtime and copies helper binaries from `third-parties/bin` plus system DLLs.
- If using Qt Online Installer, set `-DCMAKE_PREFIX_PATH` to your Qt directory and ensure CryptoPP & libxml2 are discoverable.

## 🗂️ Project Structure

```
src/
  app/            # Application core, singletons, startup
  base/           # Player, downloads, search, library management
  providers/      # Streaming providers (HiAnime, IYF, AnimePahe, ...)
  ui/qml/         # QML UI (pages, components, player)
resources/        # Icons, fonts, images
third-parties/    # Binaries and headers (libmpv, ffmpeg, etc.)
```

## 🤝 Contributing

- Fork → branch → PR with clear description and Windows testing
- Ideas welcome: new providers, UI/UX polish, performance, docs

## 📄 License

MIT License. See `LICENSE`.

## 🙏 Acknowledgments

- libmpv, Qt, N_m3u8DL-RE, yt-dlp
- All contributors

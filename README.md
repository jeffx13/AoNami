# 🎬 ShowStream - Advanced Video Player & Streaming Platform

[![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)](https://www.microsoft.com/windows)
[![Qt](https://img.shields.io/badge/Qt-41CD52?style=for-the-badge&logo=qt&logoColor=white)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![MPV](https://img.shields.io/badge/MPV-000000?style=for-the-badge&logo=mpv&logoColor=white)](https://mpv.io/)

> **ShowStream** is a powerful, feature-rich video player built with Qt QML and powered by [libmpv](https://mpv.io/). Enjoy seamless playback of local files and online streaming content with advanced features like downloading, playlist management, and multi-provider support.

## ✨ Features

### 🎥 **Multi-Source Playback**
- **Local Files**: Play videos from your computer with full format support
- **Online Streaming**: Stream content from multiple providers
- **Download Support**: Download videos when supported by the source
- **M3U8 Playlists**: Advanced handling with [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-CLI)

### 🎮 **Advanced Player Controls**
- **Smart Playback**: Resume from where you left off
- **Speed Control**: Adjust playback speed from 0.1x to 2.0x
- **Picture-in-Picture**: Multi-tasking made easy
- **Subtitle Support**: Toggle and manage subtitles
- **Volume Control**: Precise audio management

### 📚 **Content Management**
- **Library System**: Organize and manage your media collection
- **Playlist Support**: Create and manage custom playlists
- **Search Functionality**: Find content across multiple providers
- **Episode Tracking**: Never lose your place in series

### 🌐 **Multi-Provider Support**
- **AllAnime**: Anime streaming
- **Bilibili**: Chinese video platform integration
- **AnimePahe**: Anime streaming service
- **IYF**: Additional streaming provider
- **Seedbox**: High-quality content delivery

### 🎨 **Modern UI/UX**
- **Qt QML Interface**: Beautiful, responsive design
- **Custom Controls**: Intuitive keyboard shortcuts
- **Responsive Layout**: Adapts to different screen sizes
- **Details Enhancements**: Click poster on Info page to view a larger image; popups close on outside click

## 🚀 Getting Started

### Prerequisites
- Windows 10/11
- CMake 3.16+
- Qt 6.5+ (Quick, QuickControls2, Qml, Concurrent, Core5Compat, Sql)
- MSYS2 MinGW64 toolchain (recommended on Windows)
- Dependencies: CryptoPP, libxml2

### Installation (Binary)
1. Download the latest Windows build if available
2. Extract the archive
3. Run `ShowStream.exe`

### Quick Start
1. **Local Files**: Drag & drop files/folders onto the player, or press `E` to open a file, `Ctrl+E` to open a folder
2. **Online Content**: Use the Search page to explore providers and play results
3. **Downloads**: Use the Download page or in-context actions where supported
4. **Library**: Add content and manage via the Library page

## 🎯 Player Controls

### 🎮 **Essential Controls**

| **Action** | **Keyboard Shortcut** | **Description** |
|------------|----------------------|-----------------|
| **Play/Pause** | `Space` / `Clear` | Toggle playback |
| **Mute** | `M` | Toggle audio |
| **Fullscreen** | `F` | Enter/exit fullscreen (Esc exits) |
| **PiP Mode** | `Ctrl+A` | Picture-in-Picture |
| **Volume Up** | `↑` / `Q` | Increase volume (+5) |
| **Volume Down** | `↓` / `A` | Decrease volume (-5) |

### ⏯️ **Playback Control**

| **Action** | **Keyboard Shortcut** | **Description** |
|------------|----------------------|-----------------|
| **Seek Backward** | `Z` / `←` | -5 seconds |
| **Seek Forward** | `X` / `→` | +5 seconds |
| **Fast Backward** | `Ctrl+Z` / `End` | -90 seconds |
| **Fast Forward** | `Ctrl+X` / `Page Down` | +90 seconds |
| **Speed Up** | `+` / `D` | +0.1x speed |
| **Speed Down** | `-` / `S` | -0.1x speed |
| **Reset Speed** | `R` | Toggle between 1.0x and 2.0x |
| **Double Speed (Hold)** | `Shift` | Temporarily doubles playback speed |

### 📁 **File & Navigation**

| **Action** | **Keyboard Shortcut** | **Description** |
|------------|----------------------|-----------------|
| **Open File** | `E` | Open file dialog |
| **Open Folder** | `Ctrl+E` | Open folder dialog |
| **Paste Link to Play** | `Ctrl+V` | Paste URL from clipboard |
| **Copy Current Link** | `Ctrl+C` | Copy video URL |
| **Reload** | `Ctrl+R` | Reload current item |
| **Toggle Playlist** | `P` / `W` | Show/hide playlist sidebar |
| **Previous Item/Playlist** | `Ctrl+S` / `Ctrl+Shift+S` | Load previous item / playlist |
| **Next Item/Playlist** | `Ctrl+D` / `Ctrl+Shift+D` | Load next item / playlist |
### 🧭 App Navigation (Window)

| **Action** | **Shortcut** |
|------------|--------------|
| Go to page by index | `1`..`6` |
| Cycle pages (next/prev) | `Ctrl+Tab` / `Ctrl+Shift+Tab` |
| Back/Forward in history | `Alt+Left` / `Alt+Right` |
| Minimize to taskbar | `Ctrl+Q` |
| Close window | `Ctrl+W` |

### 🔎 Explorer Page Shortcuts

- **Search**: `Enter`
- **Focus search**: `/`
- **Latest/Popular**: `L` / `P`
- **Cycle provider**: `Tab` (also closes open provider popup)
- **Scroll**: `Up` / `Down`

### 📚 Library Page Shortcuts

- **Refresh unwatched episodes**: `Ctrl+R`
- **Cycle library type**: `Tab` (also closes open combo)

## 🛠️ Development

### Tech Stack
- **Frontend**: Qt QML, C++
- **Media Engine**: libmpv
- **Network**: Qt Network
- **Build System**: CMake
- **Platform**: Windows (Qt-based)

### Project Structure
```
ShowStream/
├── src/
│   ├── app/           # Application core
│   ├── base/          # Core functionality
│   ├── ui/            # User interface
│   └── providers/     # Streaming providers
├── resources/         # Assets and images
└── third-parties/     # External dependencies
```

### Building from Source (Windows / MSYS2 MinGW64)

1) Install MSYS2 and packages (start MSYS2 MinGW64 shell):

```bash
pacman -S --needed git cmake ninja
pacman -S --needed mingw-w64-x86_64-toolchain
pacman -S --needed mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-declarative mingw-w64-x86_64-qt6-svg
pacman -S --needed mingw-w64-x86_64-cryptopp mingw-w64-x86_64-libxml2
```

2) Clone and configure:

```bash
git clone https://github.com/jeffx13/ShowStream.git
cd ShowStream
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/msys64/mingw64"
```

3) Build and install runtime deps (Qt deploy script + DLLs are handled by CMake):

```bash
cmake --build build --config Release
cmake --install build
```

Notes:
- The build links Qt6 Quick/Controls2/Qml/Concurrent/Core5Compat/Sql, `cryptopp`, and `libxml2`.
- `third-parties/bin` includes helper tools such as `ffmpeg.exe`, `yt-dlp.exe`, and `N_m3u8DL-RE.exe` and will be copied next to the executable.
- If you installed Qt via the Qt Online Installer instead of MSYS2, set `CMAKE_PREFIX_PATH` to your Qt installation and ensure CryptoPP and libxml2 are available on your PATH or via `CMAKE_PREFIX_PATH`.

## 🤝 Contributing

**We welcome contributions!** ShowStream is an open-source project that thrives on community involvement. Whether you're a developer, designer, or user, there are many ways to contribute:

### 🎯 **How to Contribute**

1. **🐛 Report Bugs**: Open an issue with detailed information
2. **💡 Suggest Features**: Share your ideas for improvements
3. **🔧 Fix Issues**: Pick up issues labeled "good first issue"
4. **📝 Improve Documentation**: Help make the project more accessible
5. **🎨 Enhance UI/UX**: Contribute to the visual design
6. **🌐 Add Providers**: Extend streaming provider support

### 🚀 **Pull Request Guidelines**

- Fork the repository
- Create a feature branch (`git checkout -b feature/amazing-feature`)
- Make your changes with clear commit messages
- Test thoroughly on Windows
- Update documentation if needed
- Submit a pull request with a detailed description

### 📋 **Development Areas**

- **New Providers**: Add support for additional streaming services
- **UI Improvements**: Enhance the user interface and experience
- **Performance**: Optimize playback and loading times
- **Features**: Implement new functionality
- **Bug Fixes**: Resolve existing issues
- **Documentation**: Improve guides and help content

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [libmpv](https://mpv.io/) - Powerful media player engine
- [Qt](https://www.qt.io/) - Cross-platform application framework
- [N_m3u8DL-RE](https://github.com/nilaoda/N_m3u8DL-CLI) - M3U8 downloader
- All contributors and community members

## 📞 Support & Community

- **Issues**: [GitHub Issues](https://github.com/jeffx13/ShowStream/issues)
- **Discussions**: [GitHub Discussions](https://github.com/jeffx13/ShowStream/discussions)
- **Wiki**: [Project Wiki](https://github.com/jeffx13/ShowStream/wiki)

---

<div align="center">

**Made with ❤️ by the ShowStream Community**

[![GitHub stars](https://img.shields.io/github/stars/jeffx13/ShowStream?style=social)](https://github.com/jeffx13/ShowStream/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/jeffx13/ShowStream?style=social)](https://github.com/jeffx13/ShowStream/network/members)
[![GitHub issues](https://img.shields.io/github/issues/jeffx13/ShowStream)](https://github.com/jeffx13/ShowStream/issues)

</div>

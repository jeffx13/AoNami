# 🎬 ShowStream - Advanced Video Player & Streaming Platform

[![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)](https://www.microsoft.com/windows)
[![Qt](https://img.shields.io/badge/Qt-41CD52?style=for-the-badge&logo=qt&logoColor=white)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![MPV](https://img.shields.io/badge/MPV-000000?style=for-the-badge&logo=mpv&logoColor=white)](https://mpv.io/)

> **ShowStream** is a powerful, feature-rich video player built with Qt QML and powered by [libmpv](https://mpv.io/). Experience seamless playback of local files and online streaming content with advanced features like video downloading, playlist management, and multi-provider support.

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
- **AnimePahe**: Anime streaming service (currently unavailable)
- **IYF**: Additional streaming provider
- **Seedbox**: High-quality content delivery

### 🎨 **Modern UI/UX**
- **Qt QML Interface**: Beautiful, responsive design
- **Custom Controls**: Intuitive keyboard shortcuts
- **Dark/Light Themes**: Personalized viewing experience
- **Responsive Layout**: Adapts to different screen sizes

## 🚀 Getting Started

### Prerequisites
- Windows 10/11
- Qt 6.x
- MPV player configuration (optional)

### Installation
1. Download the latest release for Windows
2. Extract the archive to your preferred location
3. Run `ShowStream.exe`
4. Configure your MPV settings in `%APPDATA%` (optional)

### Quick Start
1. **Local Files**: Drag and drop video files or use `Ctrl+E` to browse
2. **Online Content**: Use the search function to find streaming content
3. **Downloads**: Click the download button when available
4. **Library**: Add content to your library for easy access

## 🎯 Player Controls

### 🎮 **Essential Controls**

| **Action** | **Keyboard Shortcut** | **Description** |
|------------|----------------------|-----------------|
| **Play/Pause** | `Space` / `Clear` | Toggle playback |
| **Mute** | `M` | Toggle audio |
| **Fullscreen** | `F` | Enter/exit fullscreen |
| **PiP Mode** | `Ctrl+A` | Picture-in-Picture |
| **Volume Up** | `↑` / `Q` | Increase volume (+5) |
| **Volume Down** | `↓` / `A` | Decrease volume (-5) |

### ⏯️ **Playback Control**

| **Action** | **Keyboard Shortcut** | **Description** |
|------------|----------------------|-----------------|
| **Seek Backward** | `Z` / `←` | -5 seconds |
| **Seek Forward** | `X` / `→` | +5 seconds |
| **Fast Backward** | `Ctrl+Z` / `Page Down` | -90 seconds |
| **Fast Forward** | `Ctrl+X` / `End` | +90 seconds |
| **Speed Up** | `+` / `D` | +0.1x speed |
| **Speed Down** | `-` / `S` | -0.1x speed |
| **Reset Speed** | `R` | Reset to 1.0x or 2.0x |
| **Double Speed** | `Shift` (hold) | 2x playback |

### 📁 **File & Navigation**

| **Action** | **Keyboard Shortcut** | **Description** |
|------------|----------------------|-----------------|
| **Open Folder** | `E` | Browse folders |
| **Open File** | `Ctrl+E` | File dialog |
| **Open URL** | `Ctrl+V` | Paste URL |
| **Copy Link** | `Ctrl+C` | Copy video URL |
| **Toggle Playlist** | `P` / `W` | Show/hide playlist |
| **Previous Item** | `Ctrl+S` | Load previous |
| **Next Item** | `Ctrl+D` | Load next |

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
│   ├── gui/           # User interface
│   └── providers/     # Streaming providers
├── resources/         # Assets and images
└── third-parties/     # External dependencies
```

### Building from Source
```bash
# Clone the repository
git clone https://github.com/jeffx13/ShowStream.git
cd ShowStream

# Build with CMake
mkdir build && cd build
cmake ..
make
```

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

**Kyokou** is a Windows-only video player built with Qt QML and powered by [libmpv](https://mpv.io/).  
It supports playback from both local files and online sources, with the ability to download videos when supported by the source.

**Features:**
- **Local and Online Playback:** Play videos from your computer or stream from online sources.
- **Download Support:** Download online videos when supported. (Requires source information to be provided or fetched.)
- **.m3u8 Handling:** .m3u8 playlists are managed and downloaded using the third-party tool [N_m3u8DL](https://github.com/nilaoda/N_m3u8DL-CLI), which utilizes ffmpeg internally.
- **mpv Configuration:** The player uses your local mpv configuration located in `%APPDATA%`.

# 🎬 **Player Keyboard Shortcuts** 🎮

| 🔑 **Keys**                   | 🎯 **Action**                          |
|------------------------------|-------------------------------------|
| **Playback Controls**         |                                     |
| <kbd>Space</kbd> / <kbd>Clear</kbd> | ▶️ Play / ⏸️ Pause toggle              |
| <kbd>M</kbd>                  | 🔇 Mute / 🔊 Unmute audio             |
| <kbd>/</kbd>                  | 📊 Peak progress bar (preview/screenshot) |
| <kbd>Ctrl</kbd> + <kbd>R</kbd> | 🔄 Reload                            |
| **Seek**              |                                     |
| <kbd>Z</kbd> / <kbd>←</kbd>  | ⏪ Seek backward 5 seconds            |
| <kbd>X</kbd> / <kbd>→</kbd>  | ⏩ Seek forward 5 seconds             |           |                                     |
| <kbd>Ctrl</kbd> + <kbd>Z</kbd> / <kbd>Page Down</kbd> | ⏭️ Seek forward 90 seconds          |
| <kbd>Ctrl</kbd> + <kbd>X</kbd> / <kbd>End</kbd>      | ⏮️ Seek backward 90 seconds         |
| **Navigation**               |                                     |
| <kbd>Ctrl</kbd> + <kbd>S</kbd> | ⏮️ Load previous item                |
| <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>S</kbd> | ⏮️⏮️ Load previous playlist          |
| <kbd>Ctrl</kbd> + <kbd>D</kbd> | ⏭️ Load next item                   |
| <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>D</kbd> | ⏭️⏭️ Load next playlist              |
| **Volume**           |                                     |
| <kbd>↑</kbd> / <kbd>Q</kbd>  | 🔊 Increase volume (+5)              |
| <kbd>↓</kbd> / <kbd>A</kbd>  | 🔉 Decrease volume (-5)              |
| **Speed**             |                                     |
| <kbd>+</kbd> / <kbd>D</kbd>  | ⏩ Increase speed +0.1x (or +0.2x with <kbd>Shift</kbd>) |
| <kbd>-</kbd> / <kbd>S</kbd>  | ⏪ Decrease speed −0.1x (or −0.2x with <kbd>Shift</kbd>) |
| <kbd>R</kbd>                 | 🔄 Reset to 1.0x if faster, else set to 2.0x |
| <kbd>Shift</kbd> (hold)       | 🚀 Enable double speed mode          |
| **Subtitles**                 |                                     |
| <kbd>C</kbd>                 | 💬 Toggle subtitles on/off (status shown) |
| **Fullscreen / PiP**          |                                     |
| <kbd>F</kbd>                 | ⛶ Toggle fullscreen / Exit PiP if active |
| <kbd>Esc</kbd>               | ❌ Exit PiP if active, else exit fullscreen |
| <kbd>Ctrl</kbd> + <kbd>A</kbd> | 🔳 Toggle PiP mode                   |
| **Playlist & UI**             |                                     |
| <kbd>P</kbd> / <kbd>W</kbd>  | 📜 Toggle playlist sidebar visibility   |
| <kbd>Tab</kbd> / <kbd>*</kbd> | 🆔 Show current playing video name    |
| <kbd>V</kbd>                 | 📋 Toggle server list popup          |
| **File / URL**               |                                     |
| <kbd>E</kbd>                 | 📂 Open folder dialog                |
| <kbd>Ctrl</kbd> + <kbd>E</kbd> | 📁 Open file dialog                 |
| <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>E</kbd> | 📂 Open download directory externally |
| <kbd>Ctrl</kbd> + <kbd>V</kbd> | 📥 Open URL                        |
| <kbd>Ctrl</kbd> + <kbd>C</kbd> | 🔗 Copy video link                  |


**Note:**  
Currently, Kyokou is available for Windows only.

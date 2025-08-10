**Kyokou** is a Windows-only video player built with Qt QML and powered by [libmpv](https://mpv.io/).  
It supports playback from both local files and online sources, with the ability to download videos when supported by the source.

**Features:**
- **Local and Online Playback:** Play videos from your computer or stream from online sources.
- **Download Support:** Download online videos when supported. (Requires source information to be provided or fetched.)
- **.m3u8 Handling:** .m3u8 playlists are managed and downloaded using the third-party tool [N_m3u8DL](https://github.com/nilaoda/N_m3u8DL-CLI), which utilizes ffmpeg internally.
- **mpv Configuration:** The player uses your local mpv configuration located in `%APPDATA%`.

**Controls:**

| Keys | Action |
|------|--------|
| <kbd>Space</kbd> / <kbd>Clear</kbd> | Play/pause toggle |
| <kbd>Z</kbd> / <kbd>←</kbd> | Seek backward 5 seconds |
| <kbd>X</kbd> / <kbd>→</kbd> | Seek forward 5 seconds |
| <kbd>Page Down</kbd> | Seek forward 90 seconds |
| <kbd>End</kbd> | Seek backward 90 seconds |
| <kbd>Ctrl</kbd> + <kbd>Z</kbd> | Seek backward 90 seconds |
| <kbd>Ctrl</kbd> + <kbd>X</kbd> | Seek forward 90 seconds |
| <kbd>↑</kbd> / <kbd>Q</kbd> | Increase volume by 5 units |
| <kbd>↓</kbd> / <kbd>A</kbd> | Decrease volume by 5 units |
| <kbd>M</kbd> | Mute/unmute audio |
| <kbd>+</kbd> / <kbd>D</kbd> | Increase playback speed by 0.1x (0.2x if <kbd>Shift</kbd> held) |
| <kbd>-</kbd> / <kbd>S</kbd> | Decrease playback speed by 0.1x (0.2x if <kbd>Shift</kbd> held) |
| <kbd>R</kbd> | Reset speed to 1.0 if faster than normal, else set to 2.0 |
| <kbd>Shift</kbd> (hold) | Enable double speed mode (<code>isDoubleSpeed = true</code>) |
| <kbd>C</kbd> | Toggle subtitles on/off (shows status message) |
| <kbd>F</kbd> | Toggle fullscreen / exit PiP if active |
| <kbd>Esc</kbd> | Exit PiP if active, else exit fullscreen |
| <kbd>P</kbd> / <kbd>W</kbd> | Toggle playlist bar visibility |
| <kbd>Tab</kbd> / <kbd>*</kbd> | Show current playing item name |
| <kbd>/</kbd> | Trigger mpvPlayer peak (preview/screenshot) |
| <kbd>E</kbd> | Open folder dialog |
| <kbd>V</kbd> | Toggle server list popup |
| <kbd>Ctrl</kbd> + <kbd>S</kbd> | Load previous item |
| <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>S</kbd> | Load previous playlist |
| <kbd>Ctrl</kbd> + <kbd>D</kbd> | Load next item |
| <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>D</kbd> | Load next playlist |
| <kbd>Ctrl</kbd> + <kbd>V</kbd> | Open URL |
| <kbd>Ctrl</kbd> + <kbd>R</kbd> | Reload |
| <kbd>Ctrl</kbd> + <kbd>A</kbd> | Toggle PiP mode |
| <kbd>Ctrl</kbd> + <kbd>E</kbd> | Open file dialog |
| <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>E</kbd> | Open download directory externally |
| <kbd>Ctrl</kbd> + <kbd>C</kbd> | Copy video link |


**Note:**  
Currently, Kyokou is available for Windows only.

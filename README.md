**Kyokou** is a Windows-only video player built with Qt QML and powered by [libmpv](https://mpv.io/).  
It supports playback from both local files and online sources, with the ability to download videos when supported by the source.

**Features:**
- **Local and Online Playback:** Play videos from your computer or stream from online sources.
- **Download Support:** Download online videos when supported. (Requires source information to be provided or fetched.)
- **.m3u8 Handling:** .m3u8 playlists are managed and downloaded using the third-party tool [N_m3u8DL](https://github.com/nilaoda/N_m3u8DL-CLI), which utilizes ffmpeg internally.
- **mpv Configuration:** The player uses your local mpv configuration located in `%APPDATA%`.
- **Custom Keybindings:** Some keybindings are preset within the application and are handled internally, not passed to mpv.

**Note:**  
Currently, Kyokou is available for Windows only.

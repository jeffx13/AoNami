<div align="center">
  <img src="resources/icon_wave.png" alt="AoNami" width="128" height="128"/>
  <h1>AoNami</h1>
  <p>A desktop video player and streaming browser for Windows, built on Qt/QML and libmpv.</p>

  ![Windows](https://img.shields.io/badge/Windows-0078D6?style=flat&logo=windows&logoColor=white)
  ![Qt](https://img.shields.io/badge/Qt%206-41CD52?style=flat&logo=qt&logoColor=white)
  ![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=c%2B%2B&logoColor=white)
  ![libmpv](https://img.shields.io/badge/libmpv-000000?style=flat&logo=mpv&logoColor=white)
</div>

AoNami plays local video and streams anime, TV series, movies and more from a few
online sources, keeps track of what you're watching, and can download episodes. It
started as a way to get a fast, keyboard-friendly player that stays out of the way -
mpv doing the heavy lifting, with a Qt Quick frontend on top.

## What it does

- Plays local files and online streams through libmpv, with hardware decoding
- Tracks progress - continue watching, resume position, watched/unwatched per episode
- Downloads episodes (HLS via `N_m3u8DL-RE`, muxing via `ffmpeg`)
- Skips intros and outros automatically using AniSkip
- Optional Discord rich presence so people can see what you're watching
- Subtitle/audio track selection, playback speed, picture-in-picture, fullscreen

## Running it

Grab a Windows 10/11 build, extract it anywhere, and run `AoNami.exe`. The bundled
`ffmpeg`, `yt-dlp` and `N_m3u8DL-RE` handle downloads, so there's nothing else to set up.

A few things to try:

- Drag a file or folder onto the window to play it
- `E` opens a file, `Ctrl+E` opens a folder
- The Explorer page searches online sources; the Library page is where your shows live

## Shortcuts

- **Play/Pause** - Space
- **Fullscreen** - F (Esc to exit)
- **Picture-in-Picture** - Ctrl+A
- **Seek** - Left/Right (5s), Ctrl+Z / End (-90s), Ctrl+X / PgDown (+90s)
- **Speed** - `+` / `-` (0.1x steps), `R` toggles 1.0x/2.0x, hold Shift for 2x
- **Volume** - Up/Q louder, Down/A quieter
- **Playlist** - P/W to toggle, Ctrl+S / Ctrl+D prev/next episode, Ctrl+Shift+S / Ctrl+Shift+D prev/next playlist
- **Pages** - 1-6 to jump, Ctrl+Tab / Ctrl+Shift+Tab to cycle, Alt+Left/Right for history

## Building from source

You'll need Windows 10/11, CMake 3.16+, Qt 6 (Quick, QuickControls2, Qml, Concurrent,
Core5Compat, Sql, Network), and CryptoPP, libxml2 and libmpv available to the compiler.

Using an MSYS2 MinGW64 shell:

```bash
pacman -S --needed git cmake ninja
pacman -S --needed mingw-w64-x86_64-toolchain
pacman -S --needed mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-declarative mingw-w64-x86_64-qt6-svg
pacman -S --needed mingw-w64-x86_64-cryptopp mingw-w64-x86_64-libxml2
```

Then configure and build:

```bash
git clone https://github.com/jeffx13/AoNami.git
cd AoNami
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/msys64/mingw64"
cmake --build build --config Release
cmake --install build
```

The install step deploys the Qt QML runtime and copies the helper binaries from
`third-parties/bin` alongside the executable. If you're on the Qt Online Installer
instead of MSYS2, point `-DCMAKE_PREFIX_PATH` at your Qt directory and make sure
CryptoPP and libxml2 can be found.

## Project layout

```
src/
  app/         startup, settings, logging
  core/        networking, HTML parsing, shared types
  player/      libmpv wrapper, playlist, server selection, AniSkip
  library/     watch history and the library database
  download/    download manager
  show/        show details and search
  providers/   streaming sources (AllAnime, Bilibili, AnimePahe, iyf)
  presence/    Discord rich presence
  ui/qml/      the QML interface
resources/     icons, fonts, images
third-parties/ bundled binaries and libs (libmpv, ffmpeg, ...)
```

## A note

AoNami doesn't host any content - it just talks to third-party sources that were
already public. It's a personal project meant for personal use; treat the sources
it pulls from with some respect.

## License

MIT - see [LICENSE](LICENSE).

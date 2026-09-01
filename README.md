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

Windows only. The CMake script stops with a hard error on anything else, which is less
a design decision than an honest description of where this has ever been tested.

### What you need

**Qt 6.11 for MinGW 64-bit**, from the Qt Online Installer - not the MSYS2 Qt packages.
Tick these when installing:

- The `MinGW 13.1.0 64-bit` toolchain and `CMake` + `Ninja` under *Qt > Developer and Designer Tools*
- Qt Quick, Quick Controls, Qml, Concurrent, Sql, Network, Svg, and **Qt 5 Compatibility Module**
  (the last one is easy to miss and the build won't get far without it)

**MSYS2**, but only for libxml2. Everything else comes from Qt:

```bash
pacman -S --needed mingw-w64-x86_64-libxml2
```

CMake looks for it at `C:/msys64/mingw64` and points `FindLibXml2` straight at the two
files it needs, rather than adding MSYS2 to the prefix path. That's deliberate - MSYS2
ships plenty of its own packages that you don't want turning up in a search, so don't
"helpfully" pass `-DCMAKE_PREFIX_PATH=C:/msys64/mingw64`. If your MSYS2 lives elsewhere,
change `MSYS2_ROOT` near the top of `CMakeLists.txt`.

### Fetch the binaries that aren't in the repo

The heavy runtime files are gitignored, so a fresh clone won't run until you pull them
down. This grabs `yt-dlp`, `ffmpeg` and `N_m3u8DL-RE`:

```bash
powershell -ExecutionPolicy Bypass -File scripts/fetch-deps.ps1
```

Two more it can't fetch for you:

- **`libmpv-2.dll`** into `third-parties/bin/` - from the [mpv-player-windows](https://sourceforge.net/projects/mpv-player-windows/files/libmpv/) dev builds. Without this you get a build that links and then dies on startup.
- **Shaders** into `third-parties/mpv/shaders/` - run `third-parties/mpv/dls.sh` from a shell with `curl`. Only needed if you want the `Ctrl+1`-`Ctrl+4` shader presets to do anything.

### Build it

Open `CMakeLists.txt` in Qt Creator, pick the **Desktop Qt 6.11.0 MinGW 64-bit** kit
when it asks, and hit build. That's the whole flow - there's no configure step to
remember and no arguments to pass, because everything the build needs is already
in the CMake script.

A few things worth knowing once you're in there:

- **Close the running app before rebuilding.** Windows keeps a lock on a running `.exe`,
  and the linker's way of telling you this is `ld returned 1 exit status`, which points
  at nothing useful. If a build suddenly fails after a run, this is why.
- **Release builds check your QML, Debug builds don't.** `qmlcachegen` compiles the QML
  ahead of time in Release, so typos and bad property names surface as build errors.
  Debug skips it for faster iteration, and those same mistakes wait to ambush you at
  runtime instead. Worth a Release build before you trust a QML change.
- **`mpv.conf` and `input.conf` re-sync on every build**, not just when C++ changes -
  a build target copies `third-parties/mpv/` next to the executable. Edit the configs
  in the repo, not in the build folder, or your changes evaporate.

For something you can hand to another machine, add a CMake install step under
*Projects > Build Steps*, or run `cmake --install` against the build directory once.
It pulls in the Qt QML runtime and the helper binaries and leaves the result in
`deploy/` inside your build folder.

## Project layout

```
src/
  app/         startup, settings, logging, crash handling, Discord presence
  net/         HTTP client, Cloudflare bypass, HLS proxy, HTML parsing
  media/       libmpv wrapper, playlist, server selection, AniSkip, danmaku
  library/     watch history, the library database, downloads
  providers/   streaming sources and their crypto helpers
  ui/          search, show details, subtitle search, the item models
  ui/qml/      the QML interface
resources/     icons, fonts, images
scripts/       dependency fetching and odd jobs
third-parties/ bundled binaries and libs (libmpv, ffmpeg, ...) + the mpv config
```

Providers register themselves - drop a class next to the others, call
`REGISTER_PROVIDER(Name, order)`, and it appears in the UI. The sources are all
scraped rather than official, so they break on the site's schedule rather than
yours, which is roughly the fate they all meet eventually.

## A note

AoNami doesn't host any content - it just talks to third-party sources that were
already public. It's a personal project meant for personal use; treat the sources
it pulls from with some respect.

## License

MIT - see [LICENSE](LICENSE).

# Downloads yt-dlp, ffmpeg and N_m3u8DL-RE into third-parties/bin/.
# Usage: powershell -ExecutionPolicy Bypass -File scripts/fetch-deps.ps1

$ErrorActionPreference = "Stop"
$bin = Join-Path $PSScriptRoot "..\third-parties\bin"
$tmp = Join-Path $env:TEMP "aonami-deps"
New-Item -ItemType Directory -Force -Path $bin, $tmp | Out-Null

function Grab($url, $out) {
    Write-Host "  $url"
    Invoke-WebRequest -Uri $url -OutFile $out -UseBasicParsing
}

function ExtractTo($zip, $name, $dest) {
    $into = Join-Path $tmp ([IO.Path]::GetFileNameWithoutExtension($zip))
    Expand-Archive -Path $zip -DestinationPath $into -Force
    $exe = Get-ChildItem -Path $into -Recurse -Filter $name | Select-Object -First 1
    Copy-Item $exe.FullName (Join-Path $dest $name) -Force
}

Write-Host "yt-dlp..."
Grab "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe" (Join-Path $bin "yt-dlp.exe")

Write-Host "ffmpeg..."
$z = Join-Path $tmp "ffmpeg.zip"
Grab "https://github.com/BtbN/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-win64-gpl.zip" $z
ExtractTo $z "ffmpeg.exe" $bin

Write-Host "N_m3u8DL-RE..."
$rel = Invoke-RestMethod "https://api.github.com/repos/nilaoda/N_m3u8DL-RE/releases/latest" -Headers @{ "User-Agent" = "aonami" }
$asset = $rel.assets | Where-Object { $_.name -match "win-x64" -and $_.name -match "\.zip$" } | Select-Object -First 1
$z = Join-Path $tmp "nm3u8.zip"
Grab $asset.browser_download_url $z
ExtractTo $z "N_m3u8DL-RE.exe" $bin

Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "Done -> third-parties/bin/  (yt-dlp, ffmpeg, N_m3u8DL-RE)"
Write-Host ""
Write-Host "Still needed for a runnable build (or just download a release):"
Write-Host "  libmpv-2.dll  ->  third-parties/bin/   (mpv-dev build: https://sourceforge.net/projects/mpv-player-windows/files/libmpv/)"
Write-Host "  mpv shaders   ->  third-parties/mpv/shaders/   (Anime4K: https://github.com/bloc97/Anime4K)"

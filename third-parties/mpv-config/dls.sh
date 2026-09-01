#!/bin/bash
# Fetches every shader input.conf references. Run next to mpv.conf.
#
# The non-Anime4K shaders come out of Mangayomi's own bundle rather than upstream:
# the point is to match what that app ships, and upstream ArtCNN has moved on since.
# Anime4K comes from bloc97 and is byte-identical to Mangayomi's copy.

set -e
SHADER_DIR="shaders"
mkdir -p "$SHADER_DIR"
echo "Downloading shaders to $SHADER_DIR/ ..."
echo ""

FAIL=0
check() {
  local out="$1" label="$2"
  printf "  %-38s" "$label"
  if [ -s "$out" ] && grep -q '//!HOOK' "$out"; then
    echo "OK ($(wc -c < "$out" | tr -d ' ') bytes)"
  else
    echo "FAILED"
    FAIL=$((FAIL + 1))
  fi
}

download() {
  curl -sL -o "$2" "$1"
  check "$2" "$3"
}

echo "=== Anime4K v4.0.1 (Mode A+A) ==="
A4K="https://raw.githubusercontent.com/bloc97/Anime4K/v4.0.1/glsl"
for s in Anime4K_Clamp_Highlights Anime4K_Restore_CNN_VL Anime4K_Restore_CNN_M; do
  download "$A4K/Restore/$s.glsl" "$SHADER_DIR/$s.glsl" "$s"
done
for s in Anime4K_Upscale_CNN_x2_VL Anime4K_Upscale_CNN_x2_S \
         Anime4K_AutoDownscalePre_x2 Anime4K_AutoDownscalePre_x4; do
  download "$A4K/Upscale/$s.glsl" "$SHADER_DIR/$s.glsl" "$s"
done

echo ""
echo "=== Mangayomi bundle (FSR, SGSR, NVScaler, ArtCNN, CfL) ==="
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
curl -sL -o "$TMP/mpv.zip" \
  "https://raw.githubusercontent.com/kodjodevf/mangayomi/main/assets/mangayomi_mpv.zip"
for s in FSR SGSR NVScaler ArtCNN_C4F16 CfL_Prediction; do
  unzip -qo -j "$TMP/mpv.zip" "shaders/$s.glsl" -d "$SHADER_DIR" 2>/dev/null || true
  check "$SHADER_DIR/$s.glsl" "$s"
done

echo ""
if [ "$FAIL" -gt 0 ]; then
  echo "  WARNING: $FAIL shader(s) failed - re-run or fetch manually."
else
  echo "  All shaders downloaded OK."
fi

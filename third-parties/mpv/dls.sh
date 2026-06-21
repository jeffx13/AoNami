#!/bin/bash
###############################################################################
#  download_shaders.sh                                                        #
#  All URLs verified working (tested March 2026)                              #
#  Run from your mpv config directory (next to mpv.conf)                      #
###############################################################################

set -e
SHADER_DIR="shaders"
mkdir -p "$SHADER_DIR"
echo "Downloading shaders to $SHADER_DIR/ ..."
echo ""

FAIL=0
download() {
  local url="$1"
  local out="$2"
  local label="$3"
  printf "  %-45s" "$label"
  curl -sL -o "$out" "$url"
  if head -30 "$out" | grep -q '//!HOOK\|//!DESC\|//!BIND'; then
    echo "OK ($(wc -c < "$out" | tr -d ' ') bytes)"
  else
    echo "FAILED"
    FAIL=$((FAIL + 1))
  fi
}

echo "=== Luma Upscalers ==="

download \
  "https://raw.githubusercontent.com/Artoriuz/ArtCNN/main/GLSL/ArtCNN_C4F32.glsl" \
  "$SHADER_DIR/ArtCNN_C4F32.glsl" \
  "ArtCNN_C4F32 (best luma doubler)"

download \
  "https://raw.githubusercontent.com/Artoriuz/ArtCNN/main/GLSL/ArtCNN_C4F16.glsl" \
  "$SHADER_DIR/ArtCNN_C4F16.glsl" \
  "ArtCNN_C4F16 (lighter luma doubler)"

download \
  "https://github.com/igv/FSRCNN-TensorFlow/releases/download/1.1/FSRCNNX_x2_8-0-4-1.glsl" \
  "$SHADER_DIR/FSRCNNX_x2_8-0-4-1.glsl" \
  "FSRCNNX_x2_8-0-4-1 (CNN luma upscaler)"

download \
  "https://raw.githubusercontent.com/bjin/mpv-prescalers/master/ravu-zoom-ar-r3.hook" \
  "$SHADER_DIR/ravu-zoom-ar-r3.hook" \
  "ravu-zoom-ar-r3 (arbitrary-ratio upscaler)"

echo ""
echo "=== Chroma Upscaler ==="

download \
  "https://gist.githubusercontent.com/igv/a015fc885d5c22e6891820ad89555637/raw/KrigBilateral.glsl" \
  "$SHADER_DIR/KrigBilateral.glsl" \
  "KrigBilateral (chroma upscaler)"

echo ""
echo "=== Downscaler / Correction ==="

download \
  "https://gist.githubusercontent.com/igv/36508af3ffc84410fe39761d6969be10/raw/SSimDownscaler.glsl" \
  "$SHADER_DIR/SSimDownscaler.glsl" \
  "SSimDownscaler (4K downscaler)"

download \
  "https://gist.githubusercontent.com/igv/2364ffa6e81540f29cb7ab4c9bc05b6b/raw/SSimSuperRes.glsl" \
  "$SHADER_DIR/SSimSuperRes.glsl" \
  "SSimSuperRes (upscale correction)"

echo ""
echo "=== Anime4K v4.0.1 ==="

A4K="https://raw.githubusercontent.com/bloc97/Anime4K/v4.0.1/glsl"

download "$A4K/Restore/Anime4K_Clamp_Highlights.glsl" \
  "$SHADER_DIR/Anime4K_Clamp_Highlights.glsl" \
  "Anime4K_Clamp_Highlights"

download "$A4K/Restore/Anime4K_Restore_CNN_VL.glsl" \
  "$SHADER_DIR/Anime4K_Restore_CNN_VL.glsl" \
  "Anime4K_Restore_CNN_VL"

download "$A4K/Restore/Anime4K_Restore_CNN_Soft_VL.glsl" \
  "$SHADER_DIR/Anime4K_Restore_CNN_Soft_VL.glsl" \
  "Anime4K_Restore_CNN_Soft_VL"

download "$A4K/Upscale/Anime4K_Upscale_CNN_x2_VL.glsl" \
  "$SHADER_DIR/Anime4K_Upscale_CNN_x2_VL.glsl" \
  "Anime4K_Upscale_CNN_x2_VL"

download "$A4K/Upscale/Anime4K_Upscale_CNN_x2_M.glsl" \
  "$SHADER_DIR/Anime4K_Upscale_CNN_x2_M.glsl" \
  "Anime4K_Upscale_CNN_x2_M"

download "$A4K/Upscale/Anime4K_AutoDownscalePre_x2.glsl" \
  "$SHADER_DIR/Anime4K_AutoDownscalePre_x2.glsl" \
  "Anime4K_AutoDownscalePre_x2"

download "$A4K/Upscale/Anime4K_AutoDownscalePre_x4.glsl" \
  "$SHADER_DIR/Anime4K_AutoDownscalePre_x4.glsl" \
  "Anime4K_AutoDownscalePre_x4"

echo ""
echo "========================================"
if [ "$FAIL" -gt 0 ]; then
  echo "  WARNING: $FAIL shader(s) failed!"
  echo "  Re-run or download manually."
else
  echo "  All 14 shaders downloaded OK."
fi
echo "========================================"

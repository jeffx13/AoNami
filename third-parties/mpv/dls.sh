#!/bin/bash
# Fetches every shader input.conf references. Run next to mpv.conf.
# Not fetched: ArtCNN / CuNNy (//!COMPUTE, corrupt on the legacy 'gpu' backend) and
# fixed 2x doublers like nnedi3 / ravu-lite (never fire at our ~1.3x upscale).

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

echo "=== Luma ==="

download \
  "https://github.com/igv/FSRCNN-TensorFlow/releases/download/1.1/FSRCNNX_x2_8-0-4-1.glsl" \
  "$SHADER_DIR/FSRCNNX_x2_8-0-4-1.glsl" \
  "FSRCNNX_x2_8-0-4-1"

download \
  "https://raw.githubusercontent.com/bjin/mpv-prescalers/master/ravu-zoom-ar-r3.hook" \
  "$SHADER_DIR/ravu-zoom-ar-r3.hook" \
  "ravu-zoom-ar-r3"

echo ""
echo "=== Chroma / scaling ==="

download \
  "https://gist.githubusercontent.com/igv/a015fc885d5c22e6891820ad89555637/raw/KrigBilateral.glsl" \
  "$SHADER_DIR/KrigBilateral.glsl" \
  "KrigBilateral"

download \
  "https://gist.githubusercontent.com/igv/36508af3ffc84410fe39761d6969be10/raw/SSimDownscaler.glsl" \
  "$SHADER_DIR/SSimDownscaler.glsl" \
  "SSimDownscaler"

download \
  "https://gist.githubusercontent.com/igv/2364ffa6e81540f29cb7ab4c9bc05b6b/raw/SSimSuperRes.glsl" \
  "$SHADER_DIR/SSimSuperRes.glsl" \
  "SSimSuperRes"

echo ""
echo "=== Anime4K v4.0.1 ==="

A4K="https://raw.githubusercontent.com/bloc97/Anime4K/v4.0.1/glsl"

for s in Anime4K_Clamp_Highlights \
         Anime4K_Restore_CNN_VL \
         Anime4K_Restore_CNN_M \
         Anime4K_Restore_CNN_Soft_M; do
  download "$A4K/Restore/$s.glsl" "$SHADER_DIR/$s.glsl" "$s"
done

download "$A4K/Denoise/Anime4K_Denoise_Bilateral_Mode.glsl" \
  "$SHADER_DIR/Anime4K_Denoise_Bilateral_Mode.glsl" \
  "Anime4K_Denoise_Bilateral_Mode"

for s in Anime4K_Upscale_CNN_x2_VL \
         Anime4K_Upscale_CNN_x2_S \
         Anime4K_AutoDownscalePre_x2 \
         Anime4K_AutoDownscalePre_x4; do
  download "$A4K/Upscale/$s.glsl" "$SHADER_DIR/$s.glsl" "$s"
done

echo ""
if [ "$FAIL" -gt 0 ]; then
  echo "  WARNING: $FAIL shader(s) failed - re-run or fetch manually."
else
  echo "  All shaders downloaded OK."
fi

/**
 * - Paint pattern
 * - Use wasm to paint the pattern
 * - Get inputs and move pattern
 * - Print framerate
 * - Get sound working
 * - Connect sound to controller
 * - Lock framerate
 * - Live code reload
 * - Looped live code reload
 */
const canvas = document.querySelector("canvas");
const ctx = canvas.getContext("2d");
const width = 1280, height = 720;

const GlobalBackBuffer = new Uint8ClampedArray(4 * width * height);
const ColorView = new Uint32Array(GlobalBackBuffer.buffer);
canvas.setAttribute("width", width);
canvas.setAttribute("height", height);

requestAnimationFrame(() => {
  for (let i = 0; i < ColorView.length; i++) {
    ColorView[i] = 0xFF00BE00;
  }
  ctx.putImageData(new ImageData(GlobalBackBuffer, width), 0, 0);
});

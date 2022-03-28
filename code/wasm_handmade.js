/**
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
// const width = 300, height = 300;

const GlobalBackBuffer = new Uint8ClampedArray(4 * width * height);
const ColorView = new Uint32Array(GlobalBackBuffer.buffer);
canvas.setAttribute("width", width);
canvas.setAttribute("height", height);

const KB = 1024, MB = KB * 1024, GB = MB * 1024, TB = GB * 1024;

const GameMemory = new WebAssembly.Memory({ initial: 80 });
const GameCode = fetch("/handmade.wasm")
  .then(res => res.arrayBuffer())
  .then(bytes => WebAssembly.instantiate(bytes, { env: { memory: GameMemory } }))
  .then(({ instance }) => {
    requestAnimationFrame(() => {
      const pixels = new Uint8ClampedArray(GameMemory.buffer, 0, width * height * 4);
      const pInput = instance.exports.__heap_base;
      const pPermanentStorage = pInput;
      const pTransientStorage = pInput + 64 * MB;
      instance.exports.RenderWasmGradient(pInput, width, height);
      ctx.putImageData(new ImageData(pixels, width), 0, 0);
    });
  });

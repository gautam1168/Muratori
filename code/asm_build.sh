echo "Compiling wasm module"
clang -g -O0 --target=wasm32 --no-standard-libraries -Wl,--import-memory -Wl,--export-all -Wl,--no-entry -o ../../build/handmade.wasm handmade.cpp

echo "Add index.html and platform layer"
cp ../data/index.html ../../build/index.html
cp wasm_handmade.js ../../build/wasm_handmade.js

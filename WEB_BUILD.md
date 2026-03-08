# HTML5/WebAssembly Build Guide

This guide explains how to build your game for the web using Emscripten.

## Prerequisites

### Install Emscripten

1. Download the Emscripten SDK:
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
```

2. Install and activate the latest version:
```bash
./emsdk install latest
./emsdk activate latest
```

3. Set up environment variables (required for each terminal session):
```bash
source ./emsdk_env.sh
```

Alternatively, add this to your `.bashrc` or `.zshrc` for automatic activation:
```bash
echo 'source /path/to/emsdk/emsdk_env.sh' >> ~/.bashrc
```

## Building for Web

### Quick Build
```bash
./build-web.sh
```

This will create the following files in `build-web/`:
- `chicken_potato_fps.html` - Main HTML file
- `chicken_potato_fps.js` - JavaScript loader
- `chicken_potato_fps.wasm` - WebAssembly binary
- `chicken_potato_fps.data` - Game assets (including highscores.txt)

### Manual Build
```bash
mkdir -p build-web
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DPLATFORM=Web
cmake --build build-web --config Release
```

## Testing Locally

You need a local web server to test (browsers don't allow WebAssembly from file:// URLs):

### Option 1: Python HTTP Server
```bash
cd build-web
python3 -m http.server 8000
```
Then open http://localhost:8000/chicken_potato_fps.html

### Option 2: Node.js http-server
```bash
npm install -g http-server
cd build-web
http-server -p 8000
```

### Option 3: PHP Built-in Server
```bash
cd build-web
php -S localhost:8000
```

## Deploying to Your Website

1. Upload all four files to your web server:
   - `chicken_potato_fps.html`
   - `chicken_potato_fps.js`
   - `chicken_potato_fps.wasm`
   - `chicken_potato_fps.data`

2. Make sure your server has the correct MIME types:
   - `.wasm` → `application/wasm`
   - `.js` → `application/javascript`
   - `.data` → `application/octet-stream`

3. For better performance, enable gzip compression for `.js`, `.wasm`, and `.data` files.

### Example nginx configuration:
```nginx
location /game/ {
    types {
        application/wasm wasm;
    }
    
    gzip on;
    gzip_types application/javascript application/wasm application/octet-stream;
}
```

### Example Apache .htaccess:
```apache
AddType application/wasm .wasm
AddType application/javascript .js

<IfModule mod_deflate.c>
    AddOutputFilterByType DEFLATE application/javascript
    AddOutputFilterByType DEFLATE application/wasm
</IfModule>
```

## Browser Compatibility

The game requires a modern browser with WebAssembly support:
- Chrome 57+
- Firefox 52+
- Safari 11+
- Edge 16+

## Customizing the HTML Shell

To customize the appearance of the game page, you can create a custom HTML shell template. The default Emscripten shell is quite basic. You can:

1. Copy the generated HTML file and modify it
2. Use a custom shell template with `--shell-file` option in CMakeLists.txt
3. Embed the canvas in an existing webpage using an iframe

## Troubleshooting

### "wasm streaming compile failed"
- Ensure your server is configured to serve `.wasm` files with the correct MIME type
- Check browser console for CORS errors

### Black screen or loading issues
- Check browser console for errors
- Verify all four files (html, js, wasm, data) are in the same directory
- Ensure you're using a web server (not file://)

### Performance issues
- Enable Release build mode (already default in build-web.sh)
- Consider reducing game resolution or complexity for web
- Enable gzip compression on your server

## Platform-Specific Code

The game code can detect web platform with:
```cpp
#ifdef PLATFORM_WEB
    // Web-specific code
#endif
```

Note: Networking features may be limited or require special handling on web (WebSockets, WebRTC, etc.).

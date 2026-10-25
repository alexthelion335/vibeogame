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

This creates the HTML, JavaScript, and WebAssembly files in `build-web/`. Their names include the target platform and architecture (for example, `chicken_potato_fps_web_x86.html`). A `.data` file is created only when a `highscores.txt` file is present.

The WebAssembly build uses Emscripten's browser Fetch API for the latest-version check; it does not link native libcurl. This requires the player’s browser to have network access to the GitHub Releases API. The CMake configuration also skips preloading `highscores.txt` when that optional file is absent.

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

1. Upload the generated HTML, JavaScript, and WebAssembly files to your web server. Upload the `.data` file too if the build generated one.

### Automatic GitHub Pages Deployment

The `Deploy WebAssembly to GitHub Pages` GitHub Actions workflow builds and deploys the game whenever a GitHub release is published. It can also be started manually from the repository's Actions tab. The generated game page is published as `index.html` at the Pages site root.

To enable deployment, open the repository's **Settings → Pages** page and set **Build and deployment → Source** to **GitHub Actions**. After the first successful workflow run, GitHub displays the site URL in the `github-pages` deployment environment.

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
- Verify the generated HTML, JS, and WASM files (and `.data` file if present) are in the same directory
- Ensure you're using a web server (not file://)
- `chicken_potato_fps.data` is only generated when `highscores.txt` is present; if it is absent, deploy the generated HTML, JavaScript, and WebAssembly files.

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

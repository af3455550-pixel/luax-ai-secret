const http = require('http');
const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const PORT = 3000;
const HOST = '0.0.0.0';
const ROOT_DIR = path.resolve(__dirname, '../../../../');

const MIME_TYPES = {
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'text/javascript',
    '.json': 'application/json',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.svg': 'image/svg+xml',
    '.ico': 'image/x-icon',
    '.glsl': 'text/plain',
    '.lua': 'text/plain',
    '.mat': 'application/json',
    '.scene': 'application/json'
};

const server = http.createServer((req, res) => {
    // Enable CORS
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        res.writeHead(204);
        res.end();
        return;
    }

    const url = new URL(req.url, `http://${req.headers.host}`);

    // API: Run C++ Native Engine Executable (.exe)
    if (url.pathname === '/api/run-cpp' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            let frames = 60;
            try {
                const data = JSON.parse(body);
                if (data.frames) frames = parseInt(data.frames);
            } catch (e) {}

            const cmd = `./bin/ApexEngine.exe ${frames}`;
            exec(cmd, { cwd: ROOT_DIR }, (error, stdout, stderr) => {
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({
                    success: !error,
                    command: cmd,
                    output: stdout,
                    error: stderr || (error ? error.message : null)
                }));
            });
        });
        return;
    }

    // API: Build / Recompile C++ Engine
    if (url.pathname === '/api/build-cpp' && req.method === 'POST') {
        exec('make clean && make', { cwd: ROOT_DIR }, (error, stdout, stderr) => {
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({
                success: !error,
                output: stdout,
                error: stderr || (error ? error.message : null)
            }));
        });
        return;
    }

    // API: Get Scene JSON
    if (url.pathname === '/api/scene' && req.method === 'GET') {
        const scenePath = path.join(ROOT_DIR, 'Engine/Assets/Scenes/Sanctuary_Level_01.scene');
        fs.readFile(scenePath, 'utf8', (err, data) => {
            if (err) {
                res.writeHead(500, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: err.message }));
            } else {
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(data);
            }
        });
        return;
    }

    // API: Get Lua Script
    if (url.pathname === '/api/script' && req.method === 'GET') {
        const scriptPath = path.join(ROOT_DIR, 'Engine/Assets/Scripts/PlayerController.lua');
        fs.readFile(scriptPath, 'utf8', (err, data) => {
            res.writeHead(200, { 'Content-Type': 'text/plain' });
            res.end(data || '-- Script not found');
        });
        return;
    }

    // API: Save Lua Script (Hot-reload)
    if (url.pathname === '/api/save-script' && req.method === 'POST') {
        let body = '';
        req.on('data', chunk => body += chunk);
        req.on('end', () => {
            try {
                const data = JSON.parse(body);
                const scriptPath = path.join(ROOT_DIR, 'Engine/Assets/Scripts/PlayerController.lua');
                fs.writeFileSync(scriptPath, data.code || '');
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ success: true, message: 'Script saved and hot-reloaded' }));
            } catch (err) {
                res.writeHead(500, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: err.message }));
            }
        });
        return;
    }

    // Serve Static Files
    let reqPath = url.pathname === '/' ? '/index.html' : url.pathname;
    let filePath = path.join(__dirname, 'public', reqPath);

    // If not in public, try serving from ROOT_DIR
    if (!fs.existsSync(filePath)) {
        filePath = path.join(ROOT_DIR, reqPath);
    }

    const ext = path.extname(filePath).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';

    fs.readFile(filePath, (err, content) => {
        if (err) {
            if (err.code === 'ENOENT') {
                res.writeHead(404, { 'Content-Type': 'text/plain' });
                res.end('404 Not Found');
            } else {
                res.writeHead(500, { 'Content-Type': 'text/plain' });
                res.end('500 Server Error: ' + err.code);
            }
        } else {
            res.writeHead(200, { 'Content-Type': contentType });
            res.end(content);
        }
    });
});

server.listen(PORT, HOST, () => {
    console.log(`[ApexEngine Studio] Visual Editor listening on http://${HOST}:${PORT}`);
});

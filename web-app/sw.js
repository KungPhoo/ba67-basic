const CACHE = 'wasm-app-v1';
self.addEventListener('install', (e) => {
  e.waitUntil(
    caches.open(CACHE).then((cache) => cache.addAll([
      '/', '/BA67.html', '/BA67.js', '/BA67.wasm', '/manifest.json'
    ]))
  );
  self.skipWaiting();
});
self.addEventListener('fetch', (e) => {
  e.respondWith(caches.match(e.request).then((r) => r || fetch(e.request)));
});
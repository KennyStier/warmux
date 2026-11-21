/******************************************************************************
 *  Warmux is a convivial mass murder game.
 *  Copyright (C) 2001-2011 Warmux Team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 ******************************************************************************
 * Web build: game data downloaded when first used
 *****************************************************************************/

#include "web/web_data.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// No backslashes in this JavaScript: the C preprocessor drops them.
// The left-out files are served under lazy/ next to the page, by their path
// under /data. Each one is fetched at most once; the version in the URL
// lets the browser cache them for good.
EM_JS(void, web_data_init, (), {
  const lazy = Module.warmuxLazy = { files: new Set(), pending: new Map(), version: '' };
  let lines;
  try {
    lines = FS.readFile('/data/lazy.txt', { encoding: 'utf8' }).split('\n').filter(l => l);
  } catch (e) {
    return; // everything was packed: nothing to do
  }
  lazy.version = lines.shift();
  for (const rel of lines) {
    const path = '/data/' + rel;
    lazy.files.add(path);
    const dir = path.substring(0, path.lastIndexOf('/'));
    try { FS.mkdirTree(dir); } catch (e) {}
    try { FS.writeFile(path, new Uint8Array(0)); } catch (e) {}
  }
  lazy.fetch = (path) => {
    if (!lazy.files.has(path)) return Promise.resolve();
    if (lazy.pending.has(path)) return lazy.pending.get(path);
    const url = 'lazy/' + path.substring(6) + '?v=' + lazy.version;
    const p = fetch(url, { cache: 'force-cache' })
      .then(r => { if (!r.ok) throw new Error(r.status + ' ' + url); return r.arrayBuffer(); })
      .then(buf => { FS.writeFile(path, new Uint8Array(buf)); lazy.files.delete(path); })
      .catch(e => { console.warn('Could not download', path, e); })
      .finally(() => lazy.pending.delete(path));
    lazy.pending.set(path, p);
    return p;
  };
});

EM_ASYNC_JS(void, web_data_fetch, (const char *c_path, int folder), {
  const lazy = Module.warmuxLazy;
  if (!lazy || !lazy.fetch) return;
  let path = UTF8ToString(c_path).replace(new RegExp('/+', 'g'), '/');
  let wanted = [path];
  if (folder) {
    if (!path.endsWith('/')) path += '/';
    wanted = [...lazy.files].filter(f => f.startsWith(path) && f.indexOf('/', path.length) < 0);
  }
  await Promise.all(wanted.map(lazy.fetch));
});

EM_JS(int, web_data_ready, (const char *c_path), {
  const lazy = Module.warmuxLazy;
  return !lazy || !lazy.files.has(UTF8ToString(c_path).replace(new RegExp('/+', 'g'), '/'));
});

EM_JS(void, web_data_prefetch, (const char *c_path), {
  const lazy = Module.warmuxLazy;
  if (lazy && lazy.fetch) lazy.fetch(UTF8ToString(c_path).replace(new RegExp('/+', 'g'), '/'));
});

void WebDataInit() { web_data_init(); }
void WebDataFetch(const std::string& path) { web_data_fetch(path.c_str(), 0); }
void WebDataFetchFolder(const std::string& folder) { web_data_fetch(folder.c_str(), 1); }
bool WebDataReady(const std::string& path) { return web_data_ready(path.c_str()); }
void WebDataPrefetch(const std::string& path) { web_data_prefetch(path.c_str()); }
#endif

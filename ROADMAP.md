# Roadmap

Work that has been looked into and parked for later, with what was found so
it doesn't have to be rediscovered.

## Windows port

**Status:** parked (November 2026). Assessed, not started. Blocked mainly on
having a Windows machine to test on.

**Effort:** moderate, roughly one working session to get a Windows build
out of CI, plus a round or two of fixes once someone runs it on real
Windows.

### What already helps

- Upstream shipped Windows builds. The code still has about 73
  Windows-specific sections (`WIN32` / `_WIN32`) across 36 files: data and
  config folder locations (`src/game/config.cpp`), networking differences
  (`lib/warmux/net/`), etc.
- The SDL2 port removed most platform-specific code: window, rendering,
  input, text input, clipboard and game controllers all go through SDL2,
  which handles Windows itself.
- Every dependency is packaged by MSYS2 (UCRT64): SDL2, SDL2_image,
  SDL2_mixer, SDL2_ttf, SDL2_net, SDL2_gfx, curl, libxml2, libpng, gettext.
  The existing autotools build should work there with few changes.
- An NSIS installer script, with translations, exists in
  `build/win32_setup/` (from 2011; would need updating).

### What needs work

- **Game only, not the servers.** `warmux-server` and `warmux-index-server`
  use POSIX-only features (`fork`, `chroot`, `setuid`; see
  `lib/warmux-server/src/env.cpp`, `tools/servers/`). Build with
  `--disable-servers`; servers are hosted on Linux and Windows players just
  connect.
- **Packaging:** collect `warmux.exe`, `share/warmux` and the DLLs it needs
  (about 30; `ldd` under MSYS2 lists them) into a portable zip.
- **Stale Windows code paths:** the `WIN32` sections haven't been compiled
  since 2011; expect fixes (data path discovery, `extSDL_net.cpp`'s
  non-blocking send, which has a `MSG_DONTWAIT` fallback for Windows).
- **Testing:** no Windows machine or Wine on the development box. CI can
  prove it compiles; only a real Windows run proves it works.

### Plan

1. Add a Windows job to `.github/workflows/build.yml` using
   [`msys2/setup-msys2`](https://github.com/msys2/setup-msys2) (UCRT64):
   install the `mingw-w64-ucrt-x86_64-*` dependencies, then
   `autoreconf -fi && ./configure --disable-servers && make`.
2. Package a portable zip (`warmux.exe` + data + DLLs) and attach it to
   releases in `.github/workflows/release.yml`.
3. After it has been tested on Windows, revive the NSIS installer (or build
   an MSIX).

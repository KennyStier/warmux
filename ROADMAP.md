# Roadmap

Work that has been looked into and parked for later, with what was found so
it doesn't have to be rediscovered.

## Windows port

**Status:** working; the exe has been tested on Windows 11. The installer
has been tested under Wine only (November 2026).

### What exists

- `packaging/windows/build-windows.sh` cross-compiles on Fedora with its
  MinGW packages (SDL2_net and SDL2_gfx are built from source) and makes:
  - `warmux-<version>-windows-x64-setup.exe`: NSIS installer
    (`packaging/windows/warmux.nsi`). Installs to Program Files, adds a
    Start menu shortcut (desktop shortcut optional), registers in
    Settings > Apps, removes the previous version when upgrading. The
    uninstaller deletes exactly the installed files and keeps settings.
    The installer's own screens follow the Windows display language.
  - `warmux-<version>-windows-x64.zip`: portable, runs from any folder.
- Settings go to `%APPDATA%\Warmux`.
- CI builds both on every push (`build.yml`, artifact "windows");
  `release.yml` attaches them to releases.
- Servers are not built for Windows (`--disable-servers`): they use
  POSIX-only features (`fork`, `chroot`, `setuid`). Windows players
  connect to servers hosted on Linux.
- SDL2 comes from Fedora's `sdl2-compat`, which loads `SDL3.dll` at run
  time (included).

### What needs work

- **Test the installer on real Windows 11:** install, upgrade over an
  existing install, uninstall from Settings > Apps.
- **Code signing:** unsigned, so SmartScreen warns "unknown publisher"
  until the download builds reputation. Options: a code-signing
  certificate, or Azure Trusted Signing (inexpensive, needs identity
  verification).
- **Not yet checked on Windows:** network game against a Linux player,
  non-ASCII user names in the profile path.
- Testing under Wine in a Fedora container needs the full `wine` package
  (the installer is 32-bit) and `WINEDLLOVERRIDES="mscoree,mshtml="`.

## Own index server (online lobby)

**Status:** parked. Everything needed is built and shipped (index server,
game server, client setting); only deployment and testing remain.

### What exists

- `warmux-index-server` (port 9997): the lobby that lists open games.
  Sample config: `tools/servers/index_server/warmux_index_server.conf`.
  Keep `local=true` for a standalone lobby.
- `warmux-server` (port 3826): optional dedicated game host, registers with
  an index server via `-i <host>`. Sample config:
  `tools/servers/game_server/warmux_server.conf` (set `chroot=false` unless
  running as root).
- Clients find the lobby with `WARMUX_INDEX_SERVER=host[:port]`, or by
  default if the package is built with `--with-index-server=host[:port]`.
- Two players need no server at all: one hosts from the Network menu, the
  other connects by IP.

### What needs work

- **Direct-IP network play works** (12.0.1: a full LAN game between Linux
  and Android, hosted on Linux). Not yet tried: more than two players,
  internet play, a Windows player. `lib/warmux/net/extSDL_net.cpp` reaches
  into SDL_net's private `struct _TCPsocket`; recheck it on SDL2_net
  updates.
- The host needs TCP port 3826 open; a firewall that drops it shows up as
  "The connection timed out" on the joining side (for ufw:
  `sudo ufw allow from 192.168.0.0/24 to any port 3826 proto tcp`).
- Versions are compared strictly: 12.0.x cannot join games of the original
  11.04 release. The index server's `versions=` must list the versions it
  accepts.
- Deployment: a systemd service on an always-on machine, reachable over a
  private network such as Tailscale so nothing is exposed to the internet.

### Plan

1. Test a two-machine game by direct IP on the SDL2 build; fix what breaks.
2. Write a systemd unit and config for the index server; deploy it.
3. Optionally build the packages with `--with-index-server=` so the lobby
   works with no setup.

## Publishing on Flathub

**Status:** parked. The Flatpak manifest works
(`packaging/flatpak/`, built in CI and attached to releases), under the ID
`io.github.KennyStier.Warmux`.

### Situation

- Flathub already lists **`org.gna.Warmux`**
  (https://github.com/flathub/org.gna.Warmux), actively maintained (last
  updated September 2026) but packaging the original 11.04.1 release on
  SDL 1.2 with old patches.
- Flathub generally won't accept a second listing of the same game, so a
  separate submission would likely be rejected as a duplicate.

### Plan

1. Contact the `org.gna.Warmux` maintainer (issue on the Flathub repo)
   about switching its source to this fork, so existing users get the
   update under the ID they already have.
2. If they agree, open a pull request there with our manifest renamed to
   `org.gna.Warmux` (app ID, desktop file, icons, metainfo), sourced from a
   release tag with a pinned commit, as Flathub requires.
3. Keep `packaging/flatpak/` in sync, or point it at the Flathub manifest.

## Taking over the `warmux` AUR package (releases)

**Status:** parked. `warmux-git` (latest commit) is being adopted
separately; an adoption request is pending.

### Situation

- `warmux` (https://aur.archlinux.org/packages/warmux) builds the 11.04.1
  release. It has a maintainer (`zkhr6`) and 17 votes, but was last updated
  in 2019 and has been flagged out of date since June 2024.

### Plan

1. Comment on the package asking the maintainer to update it to 12.0.0, or
   to add us as co-maintainer; offer the PKGBUILD.
2. If there's no response within about two weeks, file an orphan request
   explaining the situation.
3. The PKGBUILD would build the release tarball
   (`https://github.com/KennyStier/warmux/archive/refs/tags/v<version>.tar.gz`,
   with its sha256) instead of a git checkout, `conflicts=('warmux-git')`.

## Decision: projectile chain reactions

**Status:** waiting for a decision. Not a bug fix in the usual sense,
because it changes gameplay.

- Upstream intended explosions to detonate projectiles caught in the blast
  (a grenade next to dynamite sets it off), via
  `WeaponProjectile::SetEnergyDelta(int, bool)`, which sets `m_energy` to 0
  so that `Refresh()` explodes it.
- Upstream later added a `dealer` parameter to the base
  `PhysicalObj::SetEnergyDelta(int, Character*)` and never updated the
  projectile version, so it stopped overriding. The base version does
  nothing for projectiles (their `m_energy` stays -1), so projectiles in a
  blast are pushed but never detonate. Chain reactions have been absent
  ever since.
- Fix: change the projectile signature (and the overrides in `anvil.cpp`
  and `cluster_bomb.cpp`) to `(int delta, Character* dealer)`. See the
  FIXME in `src/weapon/weapon_launcher.h`.
- Consequence: matches with many explosives become more chaotic; parking a
  timed grenade next to another one plays differently. Worth a playtest
  before deciding.

## Fedora COPR repository

**Status:** parked. Release RPMs are built per Fedora release (44 and 45)
in `.github/workflows/release.yml` and must be downloaded by hand; they
get no updates.

### Why per release

An RPM is tied to the library sonames of the release it was built on, and
these change between releases (Fedora 45 moved to `libxml2.so.16`, Fedora
44 has `libxml2.so.2`). One RPM for all releases would mean bundling or
statically linking libraries, which is not worth it.

### Plan

1. Create a COPR project (`kennystier/warmux`) at
   https://copr.fedorainfracloud.org, with every supported Fedora release
   enabled as a chroot.
2. Build from the spec: either SCM builds from this repository (a
   `.copr/Makefile` that fills in `@VERSION@` and makes the source
   tarball), or upload an SRPM from the release workflow with
   `copr-cli` and an API token stored as a repository secret.
3. Document install in README.md:
   `sudo dnf copr enable kennystier/warmux && sudo dnf install warmux`.
4. Keep the per-release RPMs on GitHub releases as a fallback; update
   their version matrix when Fedora releases are added or reach end of
   life.

## Android port

**Status:** playable (November 2026). Tested on a Pixel 11 Pro XL
(Android 17): menus, local games against the AI, touch controls, sharp
text. CI builds a debug APK on every push (`build.yml`, artifact
"android").

### What exists

- `packaging/android/`: Gradle project from SDL2's Android template.
  `prepare.sh` downloads SDL2 2.32 and its add-on libraries, libpng and
  libxml2 (pinned checksums), copies SDL's Java glue and stages the game
  data as APK assets; CMake builds the game as `libmain.so`, reading its
  source lists from the `Makefile.am` files.
- `io.github.kennystier.warmux`, minSdk 24, arm64-v8a and x86_64,
  landscape only; version taken from `configure.ac`.
- `src/android_data.cpp` copies the data out of the APK into the app's
  storage on first start and after updates.
- Touch controls (`src/interface/touch_controls.cpp`): walk on the left;
  fire, jump and aim on the right; weapons menu, timer and camera centre
  at the top; drag to move the camera, tap to click; Back for the pause
  menu.
- "Auto" display scale aims for a 540-pixel-high game at a whole-number
  stretch (2x on 1080p, 3x on 1440p).

### What needs work

- **Translations:** gettext is not built for Android (`ENABLE_NLS` is off
  in `lib/warmux/include/config_android.h`); everything is in English.
  Needs a libintl build (or gettext-tiny) in `prepare.sh`/CMake, and the
  `.mo` files staged with the data.
- **Online lobby and update check:** curl is not built, so the downloader
  is a no-op (`lib/warmux/net/download.cpp`). Direct-IP network games
  work (joined a Linux host over LAN). Options: build curl with mbedTLS,
  or fetch over Java (`HttpURLConnection`) through JNI.
- **Distribution:** 12.0.1 ships a signed release APK (`release.yml`;
  the key is kept outside the repository and as CI secrets). Next:
  F-Droid (free, suits GPL; builds from source) and/or Google Play
  (one-time fee, recent target SDK required).
- **Storage:** the data is stored twice (in the APK and the copy, about
  110 MB each). Reading it straight from the APK through SDL would avoid
  the copy, but the game opens files by path (libxml2, fopen).
- **Touch controls:** no back jump or slow walking/aiming yet; character
  names are drawn above characters rather than behind them (they are on
  the text overlay, see "Frame rate" below).
- **Pause on background:** check that the game pauses, and keeps its
  state, when the app is switched away from or the screen turns off.

## Frame rate: 50 FPS ceiling

**Status:** known limit; the Android port reaches it (frames take about
13 ms on a Pixel 11 Pro XL).

- The physics runs in fixed 20 ms steps (`PHYS_DELTA_T_MS` in
  `src/object/physics.cpp`, `delta_t` in `src/game/time.cpp`), and
  `Game::MainLoop()` draws at most one frame per step: never more than
  50 FPS, whatever "Maximum FPS" says. On 60/120 Hz screens frames are
  shown for uneven times, a slight judder.
- **Interpolation (preferred):** draw frames between physics steps by
  blending each object's previous and current position, and the camera's.
  Gameplay stays exactly as it is (it is deterministic, which network
  games and replays rely on), but every drawn object needs a previous
  position.
- **Shorter physics steps:** less work, but walking, jumping and timers
  are tuned to 20 ms; risks changing how the game plays and breaking
  replays.

### Rendering notes (for reference)

On screens that are a whole multiple of the game size, game frames are
drawn at game resolution and stretched by the GPU, with text on a
transparent full-resolution overlay and the touch buttons as GPU sprites
(`src/graphic/video.h`). Stretching every bitmap in software instead ran
at about 25 FPS on the Pixel.

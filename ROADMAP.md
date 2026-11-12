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

- **Network play has not been tested since the SDL2 port.** Check a full
  game between two machines first. Areas to watch: the network thread,
  and `lib/warmux/net/extSDL_net.cpp`, which reaches into SDL_net's private
  `struct _TCPsocket`; confirm its layout still matches SDL2_net 2.x.
- Versions are compared strictly: 12.0.0 cannot join games of the original
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

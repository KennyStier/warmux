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

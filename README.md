# Warmux

Warmux is a free, turn-based artillery game: teams of free-software mascots
(Tux, Gnu, Firefox, Wilber and friends) take turns blasting each other across
destructible 2D maps with bazookas, grenades, baseball bats, ropes and a lot
of other questionable equipment. Play hot-seat on one PC, against the AI, or
over the network.

This repository keeps Warmux 11.04 building and playing well on current Linux
systems. Upstream development stopped in 2011; this is a maintained fork, not
the original project.

## What's different from upstream 11.04

- **Builds on modern toolchains**: GCC 16, autoconf 2.7x, automake 1.17+,
  out-of-tree builds, Arch's hardened build flags and LTO.
- **Held keys work under Wayland**: charge-up weapons (bazooka, grenades) and
  rope swinging no longer cut out after a quarter second, which happened when
  key auto-repeat arrived as release/press pairs.
- **Network play no longer depends on the defunct upstream server**. Point it
  at your own index server instead (see [Running your own servers](#running-your-own-servers)).
- **Sensible first run**: opens at the largest standard resolution that fits
  your desktop instead of the smallest one.
- **Desktop integration**: freedesktop icons at every size, a cleaned-up
  `.desktop` entry, and a working "Warmux folder" shortcut.
- The index and game servers build cleanly again (`--enable-servers`; the
  Arch package includes them).
- Assorted fixes: a buffer overflow in the server-list reader, leaked file
  handles in the downloader.

## Installing

### Arch Linux

A PKGBUILD that builds the latest commit from this repository is in
[`packaging/arch`](packaging/arch/PKGBUILD):

```sh
git clone https://github.com/KennyStier/warmux.git
cd warmux/packaging/arch
makepkg -si
```

### Building from source

Dependencies: SDL 1.2 (or `sdl12-compat`), SDL_image, SDL_mixer, SDL_ttf,
SDL_net, SDL_gfx, libcurl, libxml2, libpng and gettext, plus a C++ compiler
and autotools.

| Distro | Packages |
|---|---|
| Arch | `sdl12-compat sdl_image sdl_mixer sdl_ttf sdl_net sdl_gfx curl libxml2 libpng base-devel` |
| Debian / Ubuntu | `libsdl1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev libsdl-net1.2-dev libsdl-gfx1.2-dev libcurl4-openssl-dev libxml2-dev libpng-dev gettext autoconf automake build-essential` |

Then:

```sh
autoreconf -fi            # only needed after editing configure.ac / Makefile.am
mkdir build && cd build
../configure --prefix=/usr --enable-servers
make -j"$(nproc)"
sudo make install
```

Useful `configure` options:

| Option | Effect |
|---|---|
| `--enable-servers` | Also build `warmux-server` and `warmux-index-server` (off by default) |
| `--with-index-server=HOST[:PORT]` | Default index server for internet games |
| `--with-meta-url=URL` | Default web host publishing the index server list |
| `--enable-debug` | Debug build with extra logging |

## Controls

| Input | Action |
|---|---|
| Left / Right | Walk (and swing on the rope) |
| Up / Down | Aim |
| Space | Fire. Hold to charge weapons that have a power bar |
| Enter / Backspace / B | Jump forward / straight up / backwards |
| Tab | Next character |
| C | Re-centre camera on the active character |
| F1 to F5 | Cycle weapons in category 1 to 5 |
| Right click | Show/hide weapons menu |
| Left click | Pick a target (teleport, homing weapons, air attack) or select a character |
| Middle click / screen edges | Scroll the map |
| P | Pause |
| F10 | Toggle fullscreen |
| Esc | Leave the game |

Your settings live in `~/.config/wormux/`; replays, chat logs and custom
teams/maps go in `~/.local/share/wormux/`. (The `wormux` name predates the
rename to Warmux and is kept so existing setups keep working.)

## Running your own servers

Playing with one other person needs no server at all: one player hosts from
the **Network** menu and the other connects to their IP address
(port 3826).

For a lobby where players can browse open games, run the two servers that
ship with the game:

**Index server** (the lobby, port 9997). Copy the sample config and run it:

```sh
cp tools/servers/index_server/warmux_index_server.conf ~/warmux_index_server.conf
# edit: set my_hostname, working_dir, and keep local=true for a standalone lobby
warmux-index-server -f ~/warmux_index_server.conf
```

**Game server** (optional dedicated host, port 3826). It registers itself with
an index server:

```sh
cp tools/servers/game_server/warmux_server.conf ~/warmux_server.conf
# edit: set game_name, public=true, and chroot=false unless running as root
warmux-server -f ~/warmux_server.conf -i your.index.server
```

**Clients** then find games through your index server:

```sh
WARMUX_INDEX_SERVER=your.index.server:9997 warmux
```

or bake the address in at build time with `--with-index-server=`.

A private network such as Tailscale or WireGuard works well for this: nothing
needs to be exposed to the internet.

### Environment variables

| Variable | Purpose |
|---|---|
| `WARMUX_INDEX_SERVER` | `host[:port]` of the index server (default port 9997) |
| `WARMUX_META_URL` | Base URL of a web host publishing `server_list` (one `host:port` per line) and `last` (latest version string) |
| `WARMUX_DATADIR` | Game data directory (default `/usr/share/warmux/`) |
| `WARMUX_FONT_PATH` | Path to the TTF font |
| `WARMUX_DEBUG_INPUT` | Set to `1` to print raw keyboard events to stderr |

## Reporting bugs

Please open an issue at <https://github.com/KennyStier/warmux/issues>. For
input problems, a log from `WARMUX_DEBUG_INPUT=1 warmux 2> input.log` helps a
lot.

## Credits and license

Warmux was created by the Warmux team (formerly Wormux) between 2001 and 2011;
see [AUTHORS](AUTHORS) for the full list of contributors. Graphics, sounds and
music are by their respective authors as credited in the game.

Warmux is free software released under the GNU General Public License,
version 2 or later. See [COPYING](COPYING).

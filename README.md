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

- **Ported to SDL2**: runs natively on Wayland and X11 without the
  `sdl12-compat` layer, and feels noticeably snappier.
- **Renders at your window's size**: no resolution picker and no blurry
  upscaling. Fullscreen uses the desktop resolution (no mode switch), and
  resizing or re-tiling the window during a match just works.
- **Display scale** (Options → Graphics): *Auto* follows the desktop's scale
  factor; *100%* renders one game pixel per screen pixel, the sharpest on
  HiDPI screens; *125–200%* enlarge the interface, handy on a TV.
- **Builds on modern toolchains**: GCC 16, autoconf 2.7x, automake 1.17+,
  out-of-tree builds, Arch's hardened build flags and LTO.
- **Held keys work properly**: charge-up weapons (bazooka, grenades) and
  rope swinging no longer cut out after a quarter second.
- **Game controller support**: plug in a pad and play, menus included.
- **Better text input**: typing uses SDL2 text input (works with input
  methods and any Unicode), and paste uses the system clipboard everywhere.
- **Network play no longer depends on the defunct upstream server**. Point it
  at your own index server instead (see [Running your own servers](#running-your-own-servers)).
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

Dependencies: SDL2 (2.0.18 or newer, or `sdl2-compat`), SDL2_image,
SDL2_mixer, SDL2_ttf, SDL2_net, SDL2_gfx, libcurl, libxml2, libpng and
gettext, plus a C++ compiler and autotools.

| Distro | Packages |
|---|---|
| Arch | `sdl2-compat sdl2_image sdl2_mixer sdl2_ttf sdl2_net sdl2_gfx curl libxml2 libpng base-devel` |
| Debian / Ubuntu | `libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev libsdl2-gfx-dev libcurl4-openssl-dev libxml2-dev libpng-dev gettext autoconf automake build-essential` |

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

### Game controllers

Any controller SDL recognises (Xbox, PlayStation, Switch Pro, 8BitDo and
most others) works in the menus and in game, with no setup:

| Control | Action |
|---|---|
| Left stick / D-pad | Walk, aim; navigate menus |
| A | Fire (hold to charge); confirm in menus |
| B / X / Y | Jump / high jump / back jump (B goes back in menus) |
| RB | Next weapon in the current category |
| LB | Next weapon category |
| LT / RT | Weapon setting (e.g. timer) down / up |
| Right stick, R3 | Move the camera, re-centre on the character |
| Back | Next character |
| Start | Pause menu |

Key bindings refer to physical key positions (a US layout's positions), so
on AZERTY and similar layouts letter bindings stay where a QWERTY user would
expect them.

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

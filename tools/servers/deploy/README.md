# Deploying the online lobby

What runs on the public server (index.warmux.net):

- `warmux-index-server`, TCP 9997: lists open internet games.
- `warmux-server`, TCP 3826: a dedicated game server, listed as
  "Public Battle". Players connect out to it, so nobody needs to forward a
  port. The first player in a game leads it (map, mode, start); when it
  starts, the next player to connect gets a new game (up to `max_nb_games`).

Both run as sandboxed systemd services with throwaway users
(`DynamicUser`), no access to the rest of the system, and memory/CPU caps.

## Install (Debian 13)

    apt install build-essential pkg-config gettext libsdl2-dev \
      libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev \
      libsdl2-gfx-dev libxml2-dev libcurl4-gnutls-dev libpng-dev libfribidi-dev
    ./configure --prefix=/opt/warmux --enable-servers
    make -C lib && make -C tools/servers
    install -d /opt/warmux/bin /etc/warmux
    install -m755 tools/servers/index_server/warmux-index-server \
      tools/servers/game_server/warmux-server /opt/warmux/bin/
    install -m644 tools/servers/deploy/index_server.conf \
      tools/servers/deploy/game_server.conf /etc/warmux/
    install -m644 tools/servers/deploy/warmux-*.service /etc/systemd/system/
    install -m644 tools/servers/deploy/warmux-tmpfiles.conf /etc/tmpfiles.d/warmux.conf
    ufw allow 9997/tcp; ufw allow 3826/tcp
    systemctl daemon-reload
    systemctl enable --now warmux-index warmux-game

`versions=` in `index_server.conf` must list every game version the lobby
accepts; add new versions there when releasing. The dedicated game server
only accepts clients of its own version, so rebuild it on each release.

Check: `warmux-list-games -n 12.0.1` should list "Public Battle".

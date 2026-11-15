# warmux.net

The game's website: one static page (`index.html`, `assets/`), no build
step. Its art comes from the game's own data (menu background, team
flags, weapon icons, the bomb icon); the screenshots are real games
captured offscreen.

The download button picks the file for the visitor's system from
`/latest.json`, which the server refreshes from the GitHub API as soon as
GitHub says a release was published or changed (a webhook), and hourly in
case one is missed. Nothing needs to change on the site for a new release.

`/testing/` lists the testing builds (the `testing` pre-release, built from
the `dev` branch) from `/testing.json`, refreshed the same way, with the
changes since the last release.

## Server (the index server's VPS, Debian 13)

    apt install caddy jq
    useradd --system --no-create-home --shell /usr/sbin/nologin warmux-web
    install -d -o warmux-web -g warmux-web /var/lib/warmux-web
    install -m755 deploy/warmux-latest deploy/warmux-webhook /usr/local/bin/
    install -m644 deploy/warmux-latest.service deploy/warmux-latest.timer \
      deploy/warmux-latest.path deploy/warmux-webhook.service /etc/systemd/system/
    install -m644 deploy/Caddyfile /etc/caddy/Caddyfile
    install -d /etc/warmux
    (umask 077; openssl rand -hex 32 > /etc/warmux/webhook-secret)
    systemctl daemon-reload
    systemctl enable --now warmux-latest.timer warmux-latest.path warmux-webhook
    systemctl reload caddy
    ufw allow 80/tcp; ufw allow 443/tcp

DNS: `warmux.net` and `www.warmux.net` point to the server; Caddy gets
the HTTPS certificates itself.

The webhook: in the GitHub repository's settings, a webhook for
**Releases** only, to `https://warmux.net/hooks/release`, content type
JSON, with the contents of `/etc/warmux/webhook-secret` as its secret.
`warmux-webhook` checks the signature and touches
`/run/warmux-webhook/refresh`, which starts `warmux-latest.service`
through `warmux-latest.path`. Its log: `journalctl -u warmux-webhook`.

## Publishing changes

    rsync -a --delete index.html assets testing warmux-vps:/var/www/warmux.net/
    # after changing deploy/: install it as in "Server" above

## Preview locally

    python3 -m http.server 8765
    # latest.json is not in the repo: fetch one to preview the downloads
    curl -s https://warmux.net/latest.json > latest.json

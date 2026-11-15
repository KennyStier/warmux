# Development cycle

```
feature branch ──PR──▶ dev ──(play-tested, approved)──▶ main + tag ──▶ release
                        │
                        └─ every push: "Warmux Testing" pre-release
```

## Branches

- **`main`**: only what has been released. Every commit on it ships in a
  release, so nothing unfinished goes here.
- **`dev`**: the next release. Finished features land here, and every push
  publishes a testing build. Its version (`configure.ac`) is the coming
  release, e.g. 12.0.3.
- **Feature branches**: for anything bigger than a small fix, branched from
  `dev` and merged back with a pull request. CI builds them; nothing is
  published.
- **Hotfixes** (a bad bug in the released version): fix on `main`, release it
  as the next patch version, then merge `main` into `dev`.

## Testing builds

Every push to `dev` runs `.github/workflows/testing.yml`, which replaces the
`testing` pre-release on GitHub with new builds:

| File | |
|---|---|
| `warmux-testing-<ver>-android.apk` | separate app, `io.github.kennystier.warmux.testing` |
| `warmux-testing-<ver>-windows-x64-setup.exe` / `.zip` | installs as "Warmux Testing" |
| `warmux-testing-<ver>.flatpak` | separate app, `io.github.KennyStier.Warmux.Testing` |

`<ver>` is the coming release and the build number, e.g. `12.0.3-test42`.
GitHub's "latest release", which the website's download button uses, never
points at the pre-release. Testers get them from
[warmux.net/testing](https://warmux.net/testing/), which lists the files and
the changes since the last release (refreshed every 15 minutes).

Testing builds are configured with `--enable-testing` (Android:
`gradlew -Pwarmux.testing`; Windows: `WARMUX_TESTING=1`). That:

- adds `-testing` to the network version (`12.0.3-testing`), so they only
  play with other testing builds,
- uses the testing index server, `index.warmux.net:9998`,
- keeps settings, teams and replays apart: `~/.config/warmux-testing` and
  `~/.local/share/warmux-testing` on Linux, `Warmux Testing` in the
  profile on Windows; on Android it is a separate app anyway.

They install and run next to the released game.

## Testing servers

Same machine as the public servers, separate sandboxed services:

| Service | Port | |
|---|---|---|
| `warmux-index-testing` | 9998 | lists testing games |
| `warmux-game-testing` | 3900 | "Testing Battle", built from `dev` |

Deploy by hand after pushing to `dev` (it rebuilds on the server and
restarts both; testing games in progress are dropped):

    tools/servers/deploy/deploy-testing.sh            # origin/dev
    tools/servers/deploy/deploy-testing.sh my-branch  # any commit

Testing builds of a given release all report the same network version, so
redeploy whenever `dev` changes the network protocol, and tell testers to
update.

## Release checklist

A release needs the maintainer's go-ahead. Then:

1. On `dev`: add the release to
   `packaging/flatpak/io.github.KennyStier.Warmux.metainfo.xml`.
2. Merge `dev` into `main`, tag `v<version>` and push the tag:
   `.github/workflows/release.yml` builds every package and publishes the
   release; the website picks it up within 30 minutes.
3. Servers:
   - if the index server changed since the last release (it did for
     12.0.3: the list of battles in progress), deploy the new one first:
     new clients ask it things the old one doesn't know;
   - add the version to `versions=` in `/etc/warmux/index_server.conf`
     (and `tools/servers/deploy/index_server.conf`), restart `warmux-index`;
   - run a game server for the new version (each version needs its own,
     on its own port); keep the previous one while players update.
4. On `dev`: bump `configure.ac` (and `lib/warmux/include/config_*.h`) to
   the next version, and set `versions=` in
   `tools/servers/deploy/testing/index_server.conf` to match; redeploy the
   testing servers.

## Automatic checks

`.github/workflows/build.yml` runs on every push to `main` and `dev` and on
pull requests: native builds on Arch, Ubuntu and Fedora, Windows, Android,
Flatpak (x86_64 and ARM64 for Ubuntu and Flatpak), and **Network game
test** (`.github/ci-network-test.sh`, on x86_64 and ARM64). It runs
computer-played clients against a local index server and game servers:
two players join a lobby and are dropped from it (neither may crash),
then a whole two-player game must start, be listed as in progress, and
end. Run it locally with `sh .github/ci-network-test.sh`.

**Memory check (AddressSanitizer)** runs the same test with the game and
servers built with AddressSanitizer (`NETTEST_SANITIZE=1`), so using freed
memory fails even when a normal build happens not to crash (on phones it
does). Locally: `NETTEST_SANITIZE=1 sh .github/ci-network-test.sh` (builds
in `_asantest/`).

## Testing a network game unattended

`WARMUX_TEST_AUTOPLAY=1` hands a player's teams to the computer and keeps
pressing Ready / Start, and leaves the results screen, so a full network
game plays itself. `WARMUX_TEST_AUTOPLAY=lobby` only joins and waits in
the lobby. With a local index server and game server:

    WARMUX_INDEX_SERVER=127.0.0.1 WARMUX_TEST_AUTOPLAY=1 warmux -c127.0.0.1

(start two of those). `SDL_VIDEODRIVER=offscreen` runs them without
windows; `WARMUX_SCREENSHOT=<file.png>` saves the screen after
`WARMUX_SCREENSHOT_DELAY` ms and quits.

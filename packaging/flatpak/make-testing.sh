#!/bin/sh
# Writes the Flatpak manifest and metainfo for "Warmux Testing" (built from
# the dev branch): app ID io.github.KennyStier.Warmux.Testing, so it installs
# next to Warmux, configured with --enable-testing (testing servers, own
# settings). Run from anywhere; the files land next to this script.
set -e
cd "$(dirname "$0")"
ID=io.github.KennyStier.Warmux
NEW=$ID.Testing

sed -e "s/^id: $ID\$/id: $NEW/" \
    -e "s|^\(\s*\)- --disable-servers\$|\1- --disable-servers\n\1- --enable-testing|" \
    -e "s|^\(\s*\)- install -Dm644 .*metainfo.xml\$|\1- install -Dm644 \${FLATPAK_BUILDER_BUILDDIR}/packaging/flatpak/$NEW.metainfo.xml /app/share/metainfo/$NEW.metainfo.xml\n\1- desktop-file-edit --set-name='Warmux Testing' /app/share/applications/\${FLATPAK_ID}.desktop|" \
    $ID.yml > $NEW.yml

sed -e "s|<id>$ID</id>|<id>$NEW</id>|" \
    -e "s|<name>Warmux</name>|<name>Warmux Testing</name>|" \
    -e "s|<launchable type=\"desktop-id\">$ID.desktop</launchable>|<launchable type=\"desktop-id\">$NEW.desktop</launchable>|" \
    $ID.metainfo.xml > $NEW.metainfo.xml

grep -q "^id: $NEW\$" $NEW.yml
grep -q -- "--enable-testing" $NEW.yml
grep -q "$NEW.metainfo.xml" $NEW.yml
grep -q "<id>$NEW</id>" $NEW.metainfo.xml
echo "Wrote $PWD/$NEW.yml and $NEW.metainfo.xml"

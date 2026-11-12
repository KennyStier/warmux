# RPM package for Fedora and similar. Built by .github/workflows/release.yml:
#   rpmbuild -tb warmux-<version>.tar.gz   (with this spec at the tarball root)
# The version is substituted from the release tag.

Name:           warmux
Version:        @VERSION@
Release:        1%{?dist}
Summary:        Turn-based artillery game with free-software mascots
License:        GPL-2.0-or-later
URL:            https://github.com/KennyStier/warmux
Source0:        warmux-%{version}.tar.gz

BuildRequires:  gcc-c++ make autoconf automake gettext-devel pkgconf-pkg-config
BuildRequires:  SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel
BuildRequires:  SDL2_net-devel SDL2_gfx-devel
BuildRequires:  libcurl-devel libxml2-devel libpng-devel
Requires:       hicolor-icon-theme
# Library dependencies are generated automatically from the binaries

%description
Teams of free-software mascots take turns blasting each other across
destructible 2D maps with bazookas, grenades, baseball bats, ropes and
plenty of other questionable equipment. Play hot-seat, against the AI or
over the network. Includes the index and game servers.

%prep
%autosetup -n warmux-%{version}

%build
autoreconf -fi
%configure --enable-servers
%make_build

%install
%make_install
%find_lang warmux

%files -f warmux.lang
%license COPYING
%doc README.md AUTHORS
%{_bindir}/warmux
%{_bindir}/warmux-server
%{_bindir}/warmux-index-server
%{_bindir}/warmux-list-games
%{_datadir}/warmux/
%{_datadir}/applications/warmux.desktop
%{_datadir}/applications/warmux_files.desktop
%{_datadir}/icons/hicolor/*/apps/warmux.*
%{_mandir}/man6/warmux.6*

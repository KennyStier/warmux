/* Warmux website: picks the right download for this device, lists every
   download of the latest release (from /latest.json, refreshed on the server
   from GitHub), and fills the weapons and teams sections. */
(function () {
  "use strict";

  var RELEASES = "https://github.com/KennyStier/warmux/releases/latest";

  // ---- Icons --------------------------------------------------------------
  var ICONS = {
    windows: '<svg viewBox="0 0 24 24"><path d="M2 4.4 10 3.3v7.9H2zM11 3.1 22 1.6v9.6H11zM2 12.4h8v7.9l-8-1.1zM11 12.4h11V22l-11-1.5z"/></svg>',
    linux: '<svg viewBox="0 0 24 24"><path d="M12 1.8c-2.6 0-4 2.2-4 4.9 0 1.2.2 2.1-.4 3.3-.9 1.6-2.9 4-2.9 6.6 0 .8.2 1.4.5 1.9-.7.4-1.6.7-1.5 1.5.1 1 1.6.9 2.9 1.2 1.2.3 2.1 1 3 .8.7-.2 1.1-.8 1.2-1.4h2.4c.1.6.5 1.2 1.2 1.4.9.2 1.8-.5 3-.8 1.3-.3 2.8-.2 2.9-1.2.1-.8-.8-1.1-1.5-1.5.3-.5.5-1.1.5-1.9 0-2.6-2-5-2.9-6.6-.6-1.2-.4-2.1-.4-3.3 0-2.7-1.4-4.9-4-4.9zm-1.6 3.4c.5 0 .8.6.8 1.3 0 .3-.1.6-.2.8l-.6-.2c.1-.2.1-.4.1-.6 0-.4-.2-.6-.3-.6-.2 0-.3.3-.3.6v.3l-.6-.3v-.1c0-.7.4-1.2 1.1-1.2zm3.2 0c.7 0 1.1.5 1.1 1.2v.2l-.6.2v-.3c0-.3-.1-.6-.4-.6-.2 0-.4.3-.4.6 0 .2 0 .3.1.5l-.6.2c-.1-.2-.2-.5-.2-.8 0-.7.4-1.2 1-1.2zM12 8c.9 0 2.3.6 2.5.9.1.2-.1.5-.5.7-.6.4-1.2.8-2 .8s-1.4-.4-2-.8c-.4-.2-.6-.5-.5-.7C9.7 8.6 11.1 8 12 8zm-2.4 3.4c.6.4 1.4.9 2.4.9s1.8-.5 2.4-.9c.8 1.3 1.7 2.9 1.7 4.6 0 2.3-1.8 3.5-4.1 3.5s-4.1-1.2-4.1-3.5c0-1.7.9-3.3 1.7-4.6z"/></svg>',
    android: '<svg viewBox="0 0 24 24"><path d="M7.2 3.6 6.1 1.8l.6-.4 1.2 1.9A7.4 7.4 0 0 1 12 2.4c1.5 0 2.9.4 4.1 1L17.3 1.4l.6.4-1.1 1.8A6.8 6.8 0 0 1 19.5 9h-15a6.8 6.8 0 0 1 2.7-5.4zM9 5.6a.9.9 0 1 0 0 1.8.9.9 0 0 0 0-1.8zm6 0a.9.9 0 1 0 0 1.8.9.9 0 0 0 0-1.8zM4.5 10h15v8.2c0 .9-.7 1.6-1.6 1.6H17v2.6a1.4 1.4 0 0 1-2.8 0v-2.6H9.8v2.6a1.4 1.4 0 0 1-2.8 0v-2.6h-.9c-.9 0-1.6-.7-1.6-1.6zM2.4 10a1.4 1.4 0 0 1 1.4 1.4v5.4a1.4 1.4 0 0 1-2.8 0v-5.4C1 10.6 1.6 10 2.4 10zm19.2 0c.8 0 1.4.6 1.4 1.4v5.4a1.4 1.4 0 0 1-2.8 0v-5.4c0-.8.6-1.4 1.4-1.4z"/></svg>',
    apple: '<svg viewBox="0 0 24 24"><path d="M16.4 12.6c0-2.6 2.1-3.8 2.2-3.9-1.2-1.8-3.1-2-3.7-2-1.6-.2-3.1.9-3.9.9-.8 0-2-.9-3.4-.9-1.7 0-3.3 1-4.2 2.6-1.8 3.1-.5 7.7 1.3 10.2.9 1.2 1.9 2.6 3.2 2.6 1.3-.1 1.8-.8 3.3-.8 1.6 0 2 .8 3.4.8 1.4 0 2.3-1.3 3.1-2.5 1-1.4 1.4-2.8 1.4-2.9-.1 0-2.7-1-2.7-4.1zM13.9 5c.7-.9 1.2-2 1-3.2-1 .1-2.3.7-3 1.6-.7.8-1.2 2-1.1 3.1 1.2.1 2.3-.6 3.1-1.5z"/></svg>',
    box: '<svg viewBox="0 0 24 24"><path d="M12 1.5 2.5 6.3v11.4L12 22.5l9.5-4.8V6.3zm0 2.2 6.9 3.5L12 10.6 5.1 7.2zM4.5 8.8l6.5 3.3v7.8l-6.5-3.3zm15 0v7.8L13 19.9v-7.8z"/></svg>'
  };

  // ---- What each release file is -----------------------------------------
  // id: how this device is matched; order: place in the list
  function describe(name) {
    var m;
    if (/-windows-x64-setup\.exe$/.test(name))
      return { id: "windows", order: 1, icon: "windows", title: "Windows", kind: "Installer",
               hint: "If Windows warns about an unknown publisher, choose <b>More info</b> then <b>Run anyway</b>." };
    if (/-windows-x64\.zip$/.test(name))
      return { id: "windows-zip", order: 2, icon: "windows", title: "Windows", kind: "Portable .zip",
               hint: "Unzip anywhere and run <code>warmux.exe</code>." };
    if (/-android\.apk$/.test(name))
      return { id: "android", order: 3, icon: "android", title: "Android", kind: "APK",
               hint: "Allow your browser to install apps when Android asks." };
    if (/\.flatpak$/.test(name))
      return { id: "flatpak", order: 4, icon: "linux", title: "Linux (Flatpak)", kind: "Any distribution",
               hint: "Open it with your software center, or run <code>flatpak install --user ./" + name + "</code>" };
    if (/_amd64\.deb$/.test(name))
      return { id: "deb", order: 5, icon: "box", title: "Debian / Ubuntu", kind: ".deb package",
               hint: "Run <code>sudo apt install ./" + name + "</code>" };
    if ((m = /\.fc(\d+)\.x86_64\.rpm$/.exec(name)))
      return { id: "rpm", fedora: +m[1], order: 6 + (100 - m[1]) / 1000, icon: "box",
               title: "Fedora " + m[1], kind: ".rpm package",
               hint: "Run <code>sudo dnf install ./" + name + "</code>" };
    if (/\.pkg\.tar\.zst$/.test(name))
      return { id: "arch", order: 7, icon: "box", title: "Arch Linux", kind: "pacman package",
               hint: "Run <code>sudo pacman -U ./" + name + "</code>" };
    return null;
  }

  // ---- Which download suits this device -----------------------------------
  function detectOS() {
    var ua = navigator.userAgent || "";
    var plat = (navigator.userAgentData && navigator.userAgentData.platform) || navigator.platform || "";
    if (/Android/i.test(ua)) return "android";
    if (/iPhone|iPad|iPod/i.test(ua)) return "ios";
    if (/Mac/i.test(plat) && navigator.maxTouchPoints > 1) return "ios"; // iPadOS
    if (/Win/i.test(plat) || /Windows/i.test(ua)) return "windows";
    if (/Mac/i.test(plat) || /Mac OS X/i.test(ua)) return "mac";
    if (/CrOS/i.test(ua)) return "deb"; // Linux on ChromeOS is Debian
    if (/Ubuntu|Debian|Mint|Pop!_OS|elementary/i.test(ua)) return "deb";
    if (/Fedora/i.test(ua)) return "rpm";
    if (/Linux|X11|BSD/i.test(ua + plat)) return "flatpak";
    return "unknown";
  }

  function human(bytes) {
    if (!bytes) return "";
    var mb = bytes / 1048576;
    return (mb >= 10 ? Math.round(mb) : mb.toFixed(1)) + " MB";
  }

  function el(tag, cls, html) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (html != null) e.innerHTML = html;
    return e;
  }

  function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, function (c) {
      return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c];
    });
  }

  // ---- The big button and the list -----------------------------------------
  var os = detectOS();
  var button = document.getElementById("dl");
  var icon = document.getElementById("dl-icon");
  var title = document.getElementById("dl-title");
  var sub = document.getElementById("dl-sub");
  var hint = document.getElementById("dl-hint");

  function showGeneric() {
    icon.innerHTML = ICONS.box;
    if (os === "mac" || os === "ios") {
      icon.innerHTML = ICONS.apple;
      button.classList.add("is-unavailable");
      title.textContent = os === "mac" ? "Not on macOS yet" : "Not on iPhone or iPad yet";
      sub.textContent = "See the Windows, Linux and Android versions below";
      button.href = "#downloads";
      hint.innerHTML = "Warmux runs on Windows, Linux and Android. Help bring it to Apple devices on <a href=\"https://github.com/KennyStier/warmux\">GitHub</a>.";
    }
  }

  function render(rel) {
    var files = [];
    (rel.assets || []).forEach(function (a) {
      var d = describe(a.name);
      if (!d) return;
      d.name = a.name;
      d.size = a.size;
      d.url = a.url;
      files.push(d);
    });
    files.sort(function (a, b) { return a.order - b.order; });

    // The pick: the newest Fedora for rpm, Flatpak for other Linux
    var pick = null;
    files.forEach(function (f) {
      if (f.id !== os) return;
      if (!pick || (f.fedora || 0) > (pick.fedora || 0)) pick = f;
    });

    var version = rel.version || (rel.tag || "").replace(/^v/, "");
    document.getElementById("dl-version").textContent =
      "Version " + version + (rel.published ? " · " + new Date(rel.published).toLocaleDateString(undefined, { year: "numeric", month: "long", day: "numeric" }) : "");

    if (pick) {
      button.href = pick.url;
      icon.innerHTML = ICONS[pick.icon];
      // Short on the button ("Linux"); the details go underneath
      var linux = /^(flatpak|deb|rpm|arch)$/.test(pick.id);
      title.textContent = "Download for " + (linux ? "Linux" : pick.title);
      sub.textContent = "Version " + version + " · " + (linux ? pick.title.replace(/^Linux \(Flatpak\)$/, "Flatpak") + " · " : "")
                        + pick.kind + (pick.size ? " · " + human(pick.size) : "");
      hint.innerHTML = pick.hint;
    } else {
      showGeneric();
      if (os !== "mac" && os !== "ios") {
        button.href = "#downloads";
        title.textContent = "Download Warmux " + version;
        sub.textContent = "Pick your platform below";
      }
    }

    var list = document.getElementById("dl-list");
    list.innerHTML = "";
    files.forEach(function (f) {
      var li = el("li", "dl-item" + (f === pick ? " is-yours" : ""));
      li.appendChild(el("span", "os-icon", ICONS[f.icon]));
      var info = el("div");
      info.appendChild(el("div", "dl-name", escapeHtml(f.title) + (f === pick ? "<small>for this device</small>" : "")));
      info.appendChild(el("div", "dl-file", escapeHtml(f.kind) + " · " + escapeHtml(f.name) + (f.size ? " · " + human(f.size) : "")));
      info.appendChild(el("div", "dl-hint", f.hint));
      li.appendChild(info);
      var a = el("a", "dl-get", "Download");
      a.href = f.url;
      li.appendChild(a);
      list.appendChild(li);
    });
    if (!files.length)
      list.innerHTML = '<li class="dl-loading">No downloads found. See the <a href="' + RELEASES + '">release on GitHub</a>.</li>';
  }

  showGeneric();
  if (os === "windows") icon.innerHTML = ICONS.windows;
  else if (os === "android") icon.innerHTML = ICONS.android;
  else if (os === "deb" || os === "rpm" || os === "flatpak") icon.innerHTML = ICONS.linux;

  fetch("/latest.json", { cache: "no-cache" })
    .then(function (r) { if (!r.ok) throw new Error(r.status); return r.json(); })
    .then(render)
    .catch(function () {
      // Still useful without the list: GitHub has every file
      button.href = RELEASES;
      document.getElementById("dl-list").innerHTML =
        '<li class="dl-loading">The download list could not be loaded. Every file is on the <a href="' + RELEASES + '">release page on GitHub</a>.</li>';
    });

  // ---- Weapons belt (twice, for a seamless loop) ----------------------------
  var WEAPONS = ["bazooka_ico", "dynamite_ico", "disco_grenade_ico", "polecat_ico", "gnu_ico",
    "air_attack_ico", "anvil_ico", "baseball_ico", "clusterbomb_ico", "flame_ico", "footbomb_ico",
    "jet_pack_ico", "lowgrav_ico", "mine_ico", "parachute_ico", "rail_gun_ico", "riot_ico",
    "shotgun_ico", "slap_ico", "snipe_rifle_ico", "supertux_ico", "teleportation_ico",
    "blowtorch_ico", "airhammer_ico", "bounce_ball_ico", "cluz_ico", "m16_ico", "grapple_ico",
    "syringe_ico", "grenade_ico", "gun_ico", "suicide_ico"];
  var belt = document.getElementById("belt");
  for (var pass = 0; pass < 2; pass++) {
    WEAPONS.forEach(function (w) {
      var li = el("li");
      if (pass) li.setAttribute("aria-hidden", "true");
      var img = el("img");
      img.src = "/assets/img/w/" + w + ".png";
      img.alt = pass ? "" : w.replace(/_ico$/, "").replace(/_/g, " ");
      img.width = img.height = 48;
      img.loading = "lazy";
      li.appendChild(img);
      belt.appendChild(li);
    });
  }

  // ---- Teams ------------------------------------------------------------------
  var TEAMS = [["tux", "Tux"], ["firefox", "Firefox"], ["gnu", "GNU"], ["konqi", "Konqi"],
    ["wilber", "Wilber"], ["beastie", "Beastie"], ["bugzilla", "Bugzilla"], ["pidgin", "Pidgin"],
    ["hexley", "Hexley"], ["php", "PHP"], ["postfix", "Postfix"], ["snort", "Snort"],
    ["spip", "SPIP"], ["suse", "SUSE"], ["thunderbird", "Thunderbird"], ["workrave", "Workrave"]];
  var teams = document.getElementById("teams");
  TEAMS.forEach(function (t) {
    var li = el("li");
    var img = el("img");
    img.src = "/assets/img/team-" + t[0] + ".png";
    img.alt = "";
    img.width = img.height = 64;
    img.loading = "lazy";
    li.appendChild(img);
    li.appendChild(el("span", null, t[1]));
    teams.appendChild(li);
  });

  // ---- Screenshot picker ------------------------------------------------------
  var main = document.getElementById("shot-main");
  Array.prototype.forEach.call(document.querySelectorAll(".thumb"), function (b) {
    b.addEventListener("click", function () {
      main.src = b.getAttribute("data-src");
      main.alt = b.getAttribute("data-alt");
      Array.prototype.forEach.call(document.querySelectorAll(".thumb"), function (o) {
        o.classList.toggle("is-on", o === b);
      });
    });
  });
})();

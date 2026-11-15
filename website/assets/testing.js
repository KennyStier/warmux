/* warmux.net/testing: lists the files of the "testing" pre-release (from
   /testing.json, refreshed on the server from GitHub) and what changed since
   the last release. */
(function () {
  "use strict";

  var GITHUB = "https://github.com/KennyStier/warmux/releases/tag/testing";

  var ICONS = {
    windows: '<svg viewBox="0 0 24 24"><path d="M2 4.4 10 3.3v7.9H2zM11 3.1 22 1.6v9.6H11zM2 12.4h8v7.9l-8-1.1zM11 12.4h11V22l-11-1.5z"/></svg>',
    android: '<svg viewBox="0 0 24 24"><path d="M7.2 3.6 6.1 1.8l.6-.4 1.2 1.9A7.4 7.4 0 0 1 12 2.4c1.5 0 2.9.4 4.1 1L17.3 1.4l.6.4-1.1 1.8A6.8 6.8 0 0 1 19.5 9h-15a6.8 6.8 0 0 1 2.7-5.4zM9 5.6a.9.9 0 1 0 0 1.8.9.9 0 0 0 0-1.8zm6 0a.9.9 0 1 0 0 1.8.9.9 0 0 0 0-1.8zM4.5 10h15v8.2c0 .9-.7 1.6-1.6 1.6H17v2.6a1.4 1.4 0 0 1-2.8 0v-2.6H9.8v2.6a1.4 1.4 0 0 1-2.8 0v-2.6h-.9c-.9 0-1.6-.7-1.6-1.6zM2.4 10a1.4 1.4 0 0 1 1.4 1.4v5.4a1.4 1.4 0 0 1-2.8 0v-5.4C1 10.6 1.6 10 2.4 10zm19.2 0c.8 0 1.4.6 1.4 1.4v5.4a1.4 1.4 0 0 1-2.8 0v-5.4c0-.8.6-1.4 1.4-1.4z"/></svg>',
    linux: '<svg viewBox="0 0 24 24"><path d="M12 1.8c-2.6 0-4 2.2-4 4.9 0 1.2.2 2.1-.4 3.3-.9 1.6-2.9 4-2.9 6.6 0 .8.2 1.4.5 1.9-.7.4-1.6.7-1.5 1.5.1 1 1.6.9 2.9 1.2 1.2.3 2.1 1 3 .8.7-.2 1.1-.8 1.2-1.4h2.4c.1.6.5 1.2 1.2 1.4.9.2 1.8-.5 3-.8 1.3-.3 2.8-.2 2.9-1.2.1-.8-.8-1.1-1.5-1.5.3-.5.5-1.1.5-1.9 0-2.6-2-5-2.9-6.6-.6-1.2-.4-2.1-.4-3.3 0-2.7-1.4-4.9-4-4.9z"/></svg>'
  };

  function describe(name) {
    if (/-android\.apk$/.test(name))
      return { id: "android", order: 1, icon: "android", title: "Android", kind: "APK",
               hint: "Installs as <b>Warmux Testing</b>, next to Warmux. Allow your browser to install apps when Android asks. To get each new build automatically, add the <a href=\"https://warmux.net/testing/fdroid/repo?fingerprint=9901EEE3C6D19DD79A3D131A9F96A2CC7E3328BF1A11914E357F9C7C7420925E\">Warmux Testing F-Droid repository</a>." };
    if (/-windows-x64-setup\.exe$/.test(name))
      return { id: "windows", order: 2, icon: "windows", title: "Windows", kind: "Installer",
               hint: "Installs as <b>Warmux Testing</b>. If Windows warns about an unknown publisher, choose <b>More info</b> then <b>Run anyway</b>." };
    if (/-windows-x64\.zip$/.test(name))
      return { id: "windows-zip", order: 3, icon: "windows", title: "Windows", kind: "Portable .zip",
               hint: "Unzip anywhere and run <code>warmux.exe</code>." };
    if (/\.flatpak$/.test(name))
      return { id: "linux", order: 4, icon: "linux", title: "Linux (Flatpak)", kind: "Any distribution",
               hint: "Run <code>flatpak install --user ./" + name + "</code>" };
    return null;
  }

  function detectOS() {
    var ua = navigator.userAgent || "";
    var plat = (navigator.userAgentData && navigator.userAgentData.platform) || navigator.platform || "";
    if (/Android/i.test(ua)) return "android";
    if (/Win/i.test(plat) || /Windows/i.test(ua)) return "windows";
    if (/Linux|X11|CrOS|BSD/i.test(ua + plat) && !/iPhone|iPad|Mac/i.test(ua + plat)) return "linux";
    return "other";
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

  function human(bytes) {
    var mb = bytes / 1048576;
    return (mb >= 10 ? Math.round(mb) : mb.toFixed(1)) + " MB";
  }

  var os = detectOS();

  function render(rel) {
    var build = (rel.title || "").replace(/^Warmux Testing\s*/, "") || "testing";
    document.getElementById("dl-version").textContent = "Build " + build +
      (rel.published ? " · " + new Date(rel.published).toLocaleString(undefined,
        { year: "numeric", month: "long", day: "numeric", hour: "numeric", minute: "2-digit" }) : "");

    var files = [];
    (rel.assets || []).forEach(function (a) {
      var d = describe(a.name);
      if (!d) return;
      d.name = a.name; d.size = a.size; d.url = a.url;
      files.push(d);
    });
    files.sort(function (a, b) { return a.order - b.order; });

    var list = document.getElementById("dl-list");
    list.innerHTML = "";
    files.forEach(function (f) {
      var yours = f.id === os;
      var li = el("li", "dl-item" + (yours ? " is-yours" : ""));
      li.appendChild(el("span", "os-icon", ICONS[f.icon]));
      var info = el("div");
      info.appendChild(el("div", "dl-name", escapeHtml(f.title) + (yours ? "<small>for this device</small>" : "")));
      info.appendChild(el("div", "dl-file", escapeHtml(f.kind) + " · " + escapeHtml(f.name) + (f.size ? " · " + human(f.size) : "")));
      info.appendChild(el("div", "dl-hint", f.hint));
      li.appendChild(info);
      var a = el("a", "dl-get", "Download");
      a.href = f.url;
      li.appendChild(a);
      list.appendChild(li);
    });
    if (!files.length)
      list.innerHTML = '<li class="dl-loading">No testing build right now. See <a href="' + GITHUB + '">GitHub</a>.</li>';

    // The release notes list the commits since the last release as "- ..."
    var changes = (rel.notes || "").split("\n").filter(function (l) { return /^- /.test(l); });
    if (changes.length) {
      var ul = document.getElementById("change-list");
      changes.forEach(function (l) { ul.appendChild(el("li", null, escapeHtml(l.slice(2)))); });
      document.getElementById("changes").hidden = false;
    }
  }

  fetch("/testing.json", { cache: "no-cache" })
    .then(function (r) { if (!r.ok) throw new Error(r.status); return r.json(); })
    .then(render)
    .catch(function () {
      document.getElementById("dl-list").innerHTML =
        '<li class="dl-loading">No testing build could be loaded. See the <a href="' + GITHUB + '">testing release on GitHub</a>.</li>';
    });
})();

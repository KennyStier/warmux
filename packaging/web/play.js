// The page around the web build (shell.html): loading progress, errors, and
// the Full screen tile. Loaded before warmux.js, which reads Module.
const loading = document.getElementById('loading');
const statusLine = document.getElementById('status');
const fill = document.getElementById('fill');
const canvas = document.getElementById('canvas');
canvas.addEventListener('mousedown', () => canvas.focus());
canvas.addEventListener('contextmenu', (e) => e.preventDefault());

// The main menu's Full screen tile: the game says when it is under the
// pointer or selected, and the click or key that presses it toggles full
// screen here, where the browser allows it
function toggleFullscreen() {
  if (document.fullscreenElement) document.exitFullscreen();
  else document.documentElement.requestFullscreen().catch(() => {});
}
canvas.addEventListener('pointerup', () => {
  const tile = Module.warmuxFullscreenTile;
  if (tile && tile.hover) toggleFullscreen();
});
document.addEventListener('keyup', (e) => {
  const tile = Module.warmuxFullscreenTile;
  if (tile && tile.selected && (e.key === 'Enter' || e.key === ' ')) toggleFullscreen();
});

function fail(text) {
  loading.style.display = '';
  loading.classList.add('failed');
  statusLine.textContent = text;
}

var Module = {
  canvas: canvas,
  print: (text) => console.log(text),
  printErr: (text) => console.warn(text),
  setStatus: (text) => {
    if (!text) return;
    // Emscripten reports downloads as "Downloading data... (done/total)"
    const m = text.match(/\((\d+(?:\.\d+)?)\/(\d+)\)/);
    if (m) {
      const done = +m[1], total = +m[2];
      fill.style.width = (100 * done / total).toFixed(1) + '%';
      statusLine.textContent = `Downloading game data… ${Math.round(done / 1048576)} of ${Math.round(total / 1048576)} MB`;
    } else {
      statusLine.textContent = text.replace('...', '…');
    }
  },
  monitorRunDependencies: (left) => {
    if (left === 0) statusLine.textContent = 'Starting…';
  },
  // Hide the box once the game draws its first frame
  postRun: [],
  onRuntimeInitialized: () => {
    requestAnimationFrame(() => requestAnimationFrame(() => { loading.style.display = 'none'; canvas.focus(); }));
  },
  onAbort: () => fail('Warmux stopped because of an error. Reload the page to try again.'),
};
window.addEventListener('error', () => fail('Warmux stopped because of an error. Reload the page to try again.'));

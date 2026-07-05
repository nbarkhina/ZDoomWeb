# ZDoom Web

A web (browser) version of [GZDoom](https://github.com/ZDoom/gzdoom) 4.14.2 — the popular
GPL source port of the DOOM engine — compiled to WebAssembly with Emscripten. It
runs entirely client‑side: pick a base IWAD (Doom, Doom II, Freedoom, Plutonia,
TNT, …), optionally stack some mods (PK3/WAD), and play right in your browser.

It uses GZDoom's OpenGL ES 2 renderer (via WebGL 2) and plays MIDI music through a
bundled FluidSynth soundfont.

Supports the following features -
- Fully web based application - using WebAssembly and WebGL 2
- Load any IWAD as the Base WAD (`-iwad`)
- Stack multiple mods / PWADs / PK3s (`-file`, drag to reorder)
- Drag & drop your own WADs/mods (`.wad`, `.pk3`, `.zip`, `.ipk3`, `.pk7`)
- Host your own preset WAD/mod list (see Hosting below)
- Hardware accelerated OpenGL ES 2 renderer
- FluidSynth MIDI music (bundled soundfont)
- Gamepad / controller support
- Full screen and zoom controls
- Saved games persist in the browser (IndexedDB)
- Host the application yourself


You can try it here: https://neilb.net/gzdoom/


# Hosting

This runs entirely in the browser, so you can host it yourself with your own WADs and
mods. Just copy everything in the `dist\` folder to your web server.

To pre‑populate the **Base WAD** and **Mods** pickers, copy your files somewhere under
`dist\` (for example a `wad\` folder) and list them in `wads.js` and `mods.js`:

- `wads.js` defines `WADLIST` — the Base WAD (IWAD) dropdown, loaded with `-iwad`.
- `mods.js` defines `MODLIST` — the Mods list, loaded with `-file` in list order.

Each entry is `{ name, path, selected }`:

```javascript
var WADLIST = [
    { "name": "Doom II", "path": "wad/doom2.wad", "selected": true },
    { "name": "Freedoom: Phase 2", "path": "wad/freedoom2.wad" }
];

var MODLIST = [
    { "name": "Brutal Doom", "path": "wad/brutal.pk3", "selected": true },
    { "name": "MyHouse", "path": "wad/myhouse.pk3" }
];
```

- `name` is the label shown in the dropdown.
- `path` is the URL to the file on your server (relative to `index.html`).
- `selected: true` pre‑loads that entry when the page opens.

Both lists are optional. If a list is empty, that picker is hidden and users can still
drag & drop their own WADs/mods (or use the browse links). The selected WAD is fetched
over HTTP and loaded into the emulator at launch — it is never uploaded anywhere.

## Autoload a single WAD

If you'd rather have the page boot **straight into one game** — no Base WAD / Mods
pickers and no clicking **Play** — set `WADURL` in `settings.js`. When it's set, the
loader interface is hidden and the game launches automatically as soon as the runtime
is ready; the loading progress bar and status text are still shown.

Edit `settings.js` (a sibling of `index.html`):

```javascript
var GZDOOMSETTINGS = {
    CLOUDSAVEURL: "",

    // Autoload: boot straight into this base IWAD, hiding the picker UI.
    WADURL:  "wad/doom2.wad",
    // Optional comma-separated list of mods, loaded on top in order (-file).
    MODURLS: "wad/brutal.pk3, wad/myhouse.pk3"
};
```

- `WADURL` — a single base IWAD, loaded with `-iwad`. Leave it empty (`""`) to use the
  normal picker / drag-and-drop interface instead.
- `MODURLS` — an optional comma-separated list of mods, loaded with `-file` in the order
  listed. Leave it empty for no mods.
- URLs may be **relative** to `index.html` (e.g. `wad/doom2.wad`) or **absolute**,
  including cloud storage — e.g.
  `https://your-storage.blob.core.windows.net/doom/doom2.wad`.

When `WADURL` is set it takes over from `wads.js` / `mods.js` (those pickers aren't
shown). You can still jump straight to a map by appending `?args=+map MAP01` to the page
URL.

# Build Instructions

You will need a Linux environment to build ZDoom Web. I used WSL on Windows but any
Linux environment will work. You need to install Emscripten first — version 3.1.49.

Install Emscripten:
- create a folder somewhere outside of this repo to install emscripten
- `git clone https://github.com/emscripten-core/emsdk.git`
- `cd emsdk`
- `./emsdk install 3.1.49`
- `./emsdk activate 3.1.49`
- `source ./emsdk_env.sh`

Then build:
- navigate back to this repo's folder
- run `make -j$(nproc)` (or just `./build.sh`)
- when it finishes, the freshly built `gzdoom.js`, `gzdoom.wasm`, and `gzdoom.data`
  are automatically copied into the `dist\` folder
- now just serve the `dist\` folder from a web server and enjoy!

The emscripten installation above is a one time setup, however you will need to run
`source ./emsdk_env.sh` from the emscripten folder every time you open a new terminal
(before running `make`). This is because the emscripten compiler does not get added
to your PATH permanently.

# Credits

ZDoom Web is a WebAssembly build of [GZDoom](https://github.com/ZDoom/gzdoom), which is
licensed under the GNU GPL v3.

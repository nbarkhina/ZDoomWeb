#!/usr/bin/env python3
"""Repack the engine resource .pk3 archives from the wadsrc* source trees.

This fork does NOT use the upstream `zipdir` host tool to build the .pk3s:
zipdir.c relies on POSIX <fts.h>/<dirent.h>, which MSVC lacks, so on this tree
the pk3s are produced with Python's stdlib `zipfile` instead. Each
`wadsrc*/static/` directory is zipped -- its *contents* placed at the archive
root -- into the matching pk3 under `pk3/`:

    wadsrc/static          -> pk3/gzdoom.pk3
    wadsrc_bm/static       -> pk3/brightmaps.pk3
    wadsrc_extra/static    -> pk3/game_support.pk3          (holds iwadinfo.txt)
    wadsrc_lights/static   -> pk3/lights.pk3
    wadsrc_widepix/static  -> pk3/game_widescreen_gfx.pk3

The committed pk3s are consumed by BOTH builds:
  * the Visual Studio build stages pk3\\*.pk3 next to gzdoom.exe
    (the CopyGameResources target in gzdoom.vcxproj), and
  * the Emscripten/web build bakes them into gzdoom.data via the Makefile's
    `--preload-file pk3/<name>.pk3@/<name>.pk3` flags.

Nothing in build.sh / make / MSBuild regenerates these pk3s, so after editing
anything under wadsrc*/static you must re-run this script to refresh the
affected pk3, then rebuild. For the web build specifically the target must be
relinked so gzdoom.data re-embeds the new pk3:

    source ./start_emc.sh && ./build.sh     # build.sh touch-forces the relink

Usage:
    python tools/repack_pk3.py                        # repack every pk3
    python tools/repack_pk3.py gzdoom                 # repack only gzdoom.pk3
    python tools/repack_pk3.py gzdoom.pk3 lights.pk3  # repack a subset

Each archive is written to a .tmp file, integrity-checked (zipfile.testzip),
then atomically moved into place. Entry order is deterministic (sorted, case-
insensitive) so a no-op repack reproduces byte-identical output and adds no git
churn. Pass only the pk3(s) you actually changed to avoid recompressing (and
thus re-diffing) the others.
"""

from __future__ import annotations

import os
import sys
import zipfile
from pathlib import Path

# repo root = parent of this script's tools/ directory
ROOT = Path(__file__).resolve().parent.parent
PK3DIR = ROOT / "pk3"

# output-pk3 -> source-tree mapping (archive root == contents of <src>/static)
MAPPING = {
    "gzdoom.pk3": "wadsrc",
    "brightmaps.pk3": "wadsrc_bm",
    "game_support.pk3": "wadsrc_extra",
    "lights.pk3": "wadsrc_lights",
    "game_widescreen_gfx.pk3": "wadsrc_widepix",
}


def repack(pk3_name: str, wadsrc: str) -> None:
    src = ROOT / wadsrc / "static"
    if not src.is_dir():
        raise SystemExit(f"error: source dir not found: {src}")

    out = PK3DIR / pk3_name
    tmp = out.parent / (pk3_name + ".tmp")

    files = []
    for dirpath, _dirs, filenames in os.walk(src):
        for name in filenames:
            full = Path(dirpath) / name
            arc = full.relative_to(src).as_posix()
            files.append((full, arc))
    files.sort(key=lambda item: item[1].lower())

    PK3DIR.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for full, arc in files:
            zf.write(full, arc)

    # integrity check before swapping into place
    with zipfile.ZipFile(tmp) as zf:
        bad = zf.testzip()
    if bad is not None:
        tmp.unlink()
        raise SystemExit(f"error: corrupt entry '{bad}' while packing {pk3_name}")

    os.replace(tmp, out)  # atomic on the same filesystem
    print(f"  {wadsrc}/static -> pk3/{pk3_name}  "
          f"({len(files)} files, {out.stat().st_size} bytes)")


def main(argv: list[str]) -> int:
    if argv:
        targets = {}
        for arg in argv:
            name = arg if arg.endswith(".pk3") else arg + ".pk3"
            if name not in MAPPING:
                known = ", ".join(sorted(MAPPING))
                raise SystemExit(f"error: unknown pk3 '{arg}'. known: {known}")
            targets[name] = MAPPING[name]
    else:
        targets = dict(MAPPING)

    print(f"Repacking {len(targets)} pk3(s) into {PK3DIR}:")
    for pk3_name, wadsrc in targets.items():
        repack(pk3_name, wadsrc)
    print("done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

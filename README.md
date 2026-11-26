# PCSX-ReARMed - yet another PCSX fork

English | [简体中文](README.zh.md)

## MOD features

### Per-game BIOS

Set the **BIOS Selection** core option to **Auto** (the default), then place a
`<name>.bios` file next to the game to use a custom BIOS for that game only.
The extension is case-insensitive (`.bios`, `.BIOS`, `.BiOs`, ...). Other
games keep using the normal BIOS.

How the lookup works:

* **M3U playlists** - `<playlist name>.bios` is the BIOS for the whole game
  and takes priority over the individual discs. This is the intended way to
  ship a translated BIOS for a multi-disc release.
* **Single-file multi-disc images (PBP)** - the image has one BIOS of its own,
  named after the image file; internal discs cannot have separate BIOS files.
* **Loose disc images** - each disc finds its own `<disc name>.bios`. Swapping
  discs at runtime updates the path, but the new BIOS is loaded only on the
  next reset, not immediately.

With **HLE** or a specific BIOS file selected, per-game BIOS files are ignored
and the standard option behavior applies. If the selected per-game BIOS is
missing or has the wrong size, the core falls back to the normal BIOS or HLE.

### Built-in cheat database

5300+ games and 55000+ cheat codes are embedded directly in the core - no
external cheat files are needed. Cheats appear as dynamic core options under
the **Cheats** category. Every entry shows its Chinese name with the original
English name underneath, so a translation can be checked at a glance.

The database is keyed by disc ID. Multi-disc releases carry the same cheat set
under every disc ID, so opening any disc of a game - or swapping discs during
play - resolves to the same cheats.

### Cheat update workflow

Replace `deps/cheat_bilingual.json` and rebuild. The JSON holds one array per
disc ID, each cheat carrying `en_string`, `cn_string` and `code` (multi-line
codes joined with `\n`); a multi-disc release simply repeats its body under
each of its IDs. The generator turns that into the embedded blob at build time:
there are no per-game caps to keep in sync, an empty pack is a hard error, and
a line that cannot be decoded back into an address and a value stops the build
instead of shipping a broken entry. `tools/gen_cheat_db.py --check` proves that
a translation pass only rewrote the names - the baseline it compares against
is a `.c` you generate from the finished pack *before* translating, so nothing
is tied to any particular source pack.

### Full simplified Chinese localization

All static core options, categories and value labels are translated to
Chinese.

### CI

No workflow runs on `push`. The quick check workflows (libretro,
libretro-emscripten, Linux, Linux armhf) only run on `pull_request`.

The Build workflow only runs on `workflow_dispatch`. One run produces one
artifact per platform/GPU combination (Android, Linux ARM/ARM64, Windows
x86/x64, Nintendo Switch), each holding a single core file under the name its
platform expects (`..._libretro_android.so`, `..._libretro.so`,
`..._libretro.dll`, `..._libretro_libnx.a`). The nightly release option
re-zips the same cores 1:1, so no archive ever contains another archive.

## Original README

![CI (Linux)](https://github.com/notaz/pcsx_rearmed/workflows/CI%20(Linux)/badge.svg)

PCSX ReARMed is yet another PCSX fork based on the PCSX-Reloaded project,
which itself contains code from PCSX, PCSX-df and PCSX-Revolution. This
version was originally ARM architecture oriented (hence the name) with
its MIPS->ARM dynamic recompilation and assembly optimizations, but more
recently it targets other architectures too. A fork of this emulator was
used in [PS Classic](https://en.wikipedia.org/wiki/PlayStation_Classic)
(without any coordination or even notification).

## Features
* ARM/ARM64 dynamic recompiler by Ari64
* [lightrec](https://github.com/pcercuei/lightrec/) dynamic recompiler for other architectures
* NEON GPU by Exophase for ARM NEON and x86 SSE2+
* PCSX4ALL GPU by Una-i/senquack for other architectures
* heavily modified P.E.Op.S. SPU
* BIOS HLE emulation (most games run without proprietary BIOS)
* libretro support

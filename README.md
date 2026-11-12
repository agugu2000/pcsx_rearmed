PCSX-ReARMed - yet another PCSX fork
====================================

## MOD Features
* **Per-game BIOS**: place a `<gamename>.bios` file next to the game image
  to use a custom BIOS for that specific game. Other games use normal BIOS.
* **Built-in cheat database**: 5300+ games with 55000+ cheat codes embedded
  directly in the core. Cheats appear as dynamic core options under the
  "Cheats" category. Every entry shows the Chinese name with the original
  English name underneath it, so a translation can be checked at a glance.
  No external cheat files needed.
* **Full simplified Chinese localization**: all static core options,
  categories and value labels translated to Chinese.
* **Cheat update workflow**: replace `deps/cheat_bilingual.json` and rebuild.
  The JSON holds one array per disc id, each cheat carrying `en_string`,
  `cn_string` and `code` (multi-line codes joined with `\n`); a multi-disc
  release simply repeats its body under each of its ids. The generator turns
  that into the embedded blob at build time: there are no per-game caps to
  keep in sync, an empty pack is a hard error, and a line that cannot be
  decoded back into an address and a value stops the build instead of
  shipping a broken entry. `tools/gen_cheat_db.py --check` proves that a
  translation pass only rewrote the names - the baseline it compares against
  is a `.c` you generate from the finished pack *before* translating, so
  nothing is tied to any particular source pack.
* **Manual CI**: the Build workflow only runs on `workflow_dispatch`. One run
  produces one artifact per platform/GPU combination, each holding a single
  core file under the name its platform expects (`..._libretro_android.so`,
  `..._libretro.so`, `..._libretro.dll`, `..._libretro_libnx.a`). The nightly
  release option re-zips the same cores 1:1, so no archive ever contains
  another archive.

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


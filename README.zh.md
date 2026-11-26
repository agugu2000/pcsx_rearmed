# PCSX-ReARMed - 又一个 PCSX 分支

[English](README.md) | 简体中文

## 本分支特性

### 按游戏加载 BIOS

把核心选项 **BIOS 选择** 设为 **自动**（默认），然后在游戏镜像旁边放一个
`<名称>.bios` 文件，即可只为这个游戏使用自定义 BIOS，其他游戏仍用普通 BIOS。
扩展名不区分大小写（`.bios`、`.BIOS`、`.BiOs` 均可）。

查找规则：

* **M3U 多盘列表** —— `<列表名>.bios` 作为整份游戏的 BIOS，优先于各盘自带的
  BIOS。这是为多盘游戏提供汉化 BIOS 的推荐方式。
* **单文件多盘镜像（PBP）** —— 整份镜像共用一个以镜像文件命名的 BIOS，
  内部各盘不能有各自的 BIOS。
* **散落的光盘镜像** —— 每张盘各自找 `<盘名>.bios`。运行中换盘会更新 BIOS
  路径，但新 BIOS 要到**下一次复位**才加载，不是立即生效。

当选项为 **HLE** 或指定了某个 BIOS 文件时，按游戏 BIOS 完全不介入，走官方逻辑。
若选定的按游戏 BIOS 不存在或尺寸不对，核心会回落到普通 BIOS 或 HLE，不会出错。

### 内置金手指数据库

5300+ 个游戏、55000+ 条金手指直接内嵌在核心里，不需要任何外部金手指文件。
金手指以动态核心选项的形式出现在 **Cheats（金手指）** 分类下。每条同时显示
中文名，下面附原名英文，便于一眼核对翻译是否准确。

数据库以光盘 ID 为键。多盘游戏在每个盘的 ID 下都带同一套金手指，因此直接
打开任意一张盘、或游戏中换盘，得到的都是同一套金手指。

### 金手指更新流程

替换 `deps/cheat_bilingual.json` 后重新编译即可。JSON 里每个盘 ID 对应一个
数组，每条金手指包含 `en_string`、`cn_string` 和 `code`（多行码用 `\n`
连接）；多盘游戏只需在每个 ID 下重复同一份内容。生成器在编译时把它转成
内嵌数据：没有需要手工同步的每游戏上限，空数据包直接报错，任何一行无法
反解成地址和数值的码也会中止编译，避免带病发布。`tools/gen_cheat_db.py
--check` 可证明翻译只改了名字——它对照的基线是翻译**之前**从定稿数据包
生成的 `.c`，不依赖任何特定来源数据包。

### 完整简体中文本地化

所有静态核心选项、分类和选项值标签均已翻译成中文。

### CI 触发方式

任何工作流都不会在 `push` 时运行。快速检查工作流（libretro、
libretro-emscripten、Linux、Linux armhf）只在 `pull_request` 时运行。

Build 工作流只在 `workflow_dispatch` 手动触发时运行。一次运行会为每个
平台/GPU 组合（Android、Linux ARM/ARM64、Windows x86/x64、Nintendo Switch）
产出一个 artifact，每个 artifact 只含一个核心文件，并按平台期望的方式命名
（`..._libretro_android.so`、`..._libretro.so`、`..._libretro.dll`、
`..._libretro_libnx.a`）。nightly release 选项只是把这些核心 1:1 重新打包，
所以不会出现压缩包套压缩包的情况。

## 原版 README

![CI (Linux)](https://github.com/notaz/pcsx_rearmed/workflows/CI%20(Linux)/badge.svg)

PCSX ReARMed 是基于 PCSX-Reloaded 项目的又一个 PCSX 分支，而 PCSX-Reloaded
本身包含来自 PCSX、PCSX-df 和 PCSX-Revolution 的代码。这个版本最初面向 ARM
架构（这也是名字的由来），带有 MIPS->ARM 动态重编译和汇编优化，不过最近也
支持其他架构。该模拟器的一个分支曾被用在
[PS Classic](https://en.wikipedia.org/wiki/PlayStation_Classic) 上（既没有
协调也没有通知原作者）。

## 特性
* Ari64 编写的 ARM/ARM64 动态重编译器
* 其他架构使用 [lightrec](https://github.com/pcercuei/lightrec/) 动态重编译器
* Exophase 编写的 NEON GPU，支持 ARM NEON 与 x86 SSE2+
* Una-i/senquack 编写的 PCSX4ALL GPU，用于其他架构
* 经过大量修改的 P.E.Op.S. SPU
* BIOS HLE 模拟（大多数游戏无需专有 BIOS 即可运行）
* 支持 libretro

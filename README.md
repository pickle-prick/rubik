# A "Unnamed" Game Project

## Development Setup Instructions

**Note: Currently, only x64 linux development is supported.**

* linux
* gcc
* glslc

## Building

Within this terminal, `cd` to the root directory of the codebase, and just run the `build.sh` script:

```
./build.sh game
```

If everything worked correctly, there will be a `build` folder in the root
level of the codebase, and it will contain a freshly-built `game`.

## Codebase Introduction

The codebase is organized into *layers*. Layers are separated either to isolate
certain problems, and to allow inclusion into various builds without needing to
pull everything in the codebase into a build. Layers correspond with folders
inside of the `src` directory. Sometimes, one folder inside of the `src`
directory will include multiple sub-layers, but the structure is intended to be
fairly flat.

Layers correspond roughly 1-to-1 with *namespaces*. The term "namespaces" in
this context does not refer to specific namespace language features, but rather
a naming convention for C-style namespaces, which are written in the codebase as
a short prefix, usually 1-3 characters, followed by an underscore. These
namespaces are used such that the layer to which certain code belongs may be
quickly understood by glancing at code. The namespaces are generally quite short
to ensure that they aren't much of a hassle to write. Sometimes, multiple sub-
layers will share a namespace. A few layers do not have a namespace, but most
do. Namespaces are either all-caps or lowercase depending on the context in
which they're used. For types, enum values, and some macros, they are
capitalized. For functions and global variables, they are lowercase.

Layers depend on other layers, but circular dependencies would break the
separability and isolation utility of layers (in effect, forming one big layer),
so in other words, layers are arranged into a directed acyclic graph.

A few layers are built to be used completely independently from the rest of the
codebase, as libraries in other codebases and projects. As such, these layers do
not depend on any other layers in the codebase.

A list of the layers in the codebase and their associated namespaces is below:
- `base` (no namespace): Universal, codebase-wide constructs. Strings, math,
  memory allocators, helper macros, command-line parsing, and so on. Depends
  on no other codebase layers.
- `draw` (`DR_`): Implements a high-level graphics drawing API for the
  game's purposes, using the underlying `render` abstraction layer. Provides
  high-level APIs for various draw commands, but takes care of batching them,
  and so on.
- `font_cache` (`FNT_`): Implements a cache of rasterized font data, both in
  CPU-side data for text shaping, and in GPU texture atlases for rasterized
  glyphs. All cache information is sourced from the `font_provider` abstraction
  layer.
- `font_provider` (`FP_`): An abstraction layer for various font file decoding
  and font rasterization backends.
- `metagen` (`MG_`): A metaprogram which is used to generate primarily code and
  data tables. Consumes Metadesk files, stored with the extension `.mdesk`, and
  generates C code which is then included by hand-written C code. Currently, it
  does not analyze the codebase's hand-written C code, but in principle this is
  possible. This allows easier & less-error-prone management of large data
  tables, which are then used to produce e.g. C `enum`s and a number of
  associated data tables. There are also a number of other generation features,
  like embedding binary files or complex multi-line strings into source code.
  This layer cannot depend on any other layer in the codebase directly,
  including `base`, because it may be used to generate code for those layers. To
  still use `base` and `os` layer features in the `metagen` program, a separate,
  duplicate version of `base` and `os` are included in this layer. They are
  updated manually, as needed. This is to ensure the stability of the
  metaprogram.
- `os/core` (`OS_`): An abstraction layer providing core, non-graphical
  functionality from the operating system under an abstract API, which is
  implemented per-target-operating-system.
- `os/gfx` (`OS_`): An abstraction layer, building on `os/core`, providing
  graphical operating system features under an abstract API, which is
  implemented per-target-operating-system.
- `render` (`R_`): An abstraction layer providing an abstract API for rendering
  using various GPU APIs under a common interface. Does not implement a high
  level drawing API - this layer is strictly for minimally abstracting on an
  as-needed basis. Higher level drawing features are implemented in the `draw`
  layer.
- `external` (no namespace): External code from other projects, which some
  layers in the codebase depend on. All external code is included and built
  directly within the codebase.
- `ui` (`UI_`): Machinery for building graphical user interfaces which could be used to build debug ui or game ui.
  Provides a core immediate mode hierarchical user interface data structure building
  API, and has helper layers for building some higher-level widgets.

## Reference

### Immediate mode ui

* https://forkingpaths.dev/posts/23-03-10/rule_based_styling_imgui.html
* https://www.youtube.com/watch?v=Z1qyvQsjK5Y

### Text Rendering

* https://www.youtube.com/watch?v=ZK7PezR1KgU

### GJK

* https://computerwebsite.net/writing/gjk

### How to write c

* https://github.com/EpicGamesExt/raddebugger

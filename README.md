# A "Unnamed" Game Engine built for understanding how 3D graphics work

![Screenshot of a 3D scene](./screenshots/003.gif)

## Development Setup Instructions

**Note: Currently, only x64 linux and Windows development are supported.**

### Linux Setup

#### 1. Installing the Required Tools

* linux
* gcc
* glslc
* vulkan-headers
* vulkan-validation-layers

#### 2. Building

Within this terminal, `cd` to the root directory of the codebase, and just run the `build.sh` script:

```
./build.sh game
```

### Windows Setup

#### 1. Installing the Required Tools

In order to work with the codebase, you'll need the [Microsoft C/C++ Build Tools
v15 (2017) or later](https://aka.ms/vs/17/release/vs_BuildTools.exe), for both
the Windows SDK and the MSVC/Clang compiler and linker. (**Note: Currently only clang can be used)

Install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) for windows, then set the %VULKAN_SDK% environment variable to your sdk installation location and include %VULKAN_SDK%/bin to your system path

#### 2. Build Environment Setup

Building the codebase can be done in a terminal which is equipped with the
ability to call either MSVC or Clang from command line.

This is generally done by calling `vcvarsall.bat x64`, which is included in the
Microsoft C/C++ Build Tools. This script is automatically called by the `x64
Native Tools Command Prompt for VS <year>` variant of the vanilla `cmd.exe`. If
you've installed the build tools, this command prompt may be easily located by
searching for `Native` from the Windows Start Menu search.

### 3. Building

Within this terminal, `cd` to the root directory of the codebase, and just run
the `build.bat` script:

```
build rubik clang
```

If everything worked correctly, there will be a `build` folder in the root
level of the codebase, and it will contain a freshly-built `rubik` executable.

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
- `draw` (`D_`): Implements a high-level graphics drawing API for the
  game's purposes, using the underlying `render` abstraction layer. Provides
  high-level APIs for various draw commands, but takes care of batching them,
  and so on.
- `font_cache` (`F_`): Implements a cache of rasterized font data, both in
  CPU-side data for text shaping, and in GPU texture atlases for rasterized
  glyphs. All cache information is sourced from the `font_provider` abstraction
  layer.
- `font_provider` (`FP_`): An abstraction layer for various font file decoding
  and font rasterization backends.
- `metagen` (`MG_`): A metaprogram which is used to generate primarily code and
  data tables(Copied from raddbg codebase). Consumes Metadesk files, stored with the extension `.mdesk`, and
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
- `rubik` (`RK_`): Machinery for building a game engine.
- `serialize` (`SE_`): An abstraction layer for serialization (currently support yml).

## Features & TODO

- [X] Debug gizmos/grid
- [X] Runtime profiler
- [X] Scripting
- [ ] Skeletal animation
    - [X] play animation
    - [ ] morph targets
- [X] Font rendering
- [X] 2D ui building blocks (immediate mode)
    - [X] basic widges (scroll, button, label, line_edit, expander, ...)
- [ ] Node tree building
    - [X] 2D
    - [X] 3D
    - [X] basic mesh primitive building block (box, sphere, plane, ...)
- [X] 3D scene editor UI
    - [X] tree viewer
    - [X] inspector
    - [X] profile panel
    - [X] stats
    - [X] dynamic drawlist (immediate mode style)
- [ ] Scene serialization and deserialization (PARTIAL)
- [ ] Multiple platform(AMD64) support
    - [ ] OS layer
        - [X] Linux
        - [X] Windows
        - [ ] MAC
    - [ ] build
        - [X] Linux
        - [X] Windows
        - [ ] MAC
- [ ] Lighting
    - [X] directional light
    - [X] point light
    - [X] spot light
    - [ ] area light
    - [ ] sky light
    - [ ] emissive light
    - [X] forward+ rendering
    - [ ] vulumetric lighting
    - [ ] shadow map
    - [X] Phong/Blinn material model
    - [ ] PBR material model
    - [ ] IBL reflection
- [ ] Assets loading
    - [X] gltf loading
    - [X] texture loading
    - [ ] multhreading to speed up
- [ ] Rendering pass
    - [X] pixel id pass
    - [X] zpre pass
    - [X] toon shading pass
    - [X] rect pass
    - [X] tiling and light culling pass (compute)
    - [ ] shadow map pass
    - [ ] blur pass (Gaussian)
    - [X] geo3d pass
    - [ ] other post effects
- [X] 2D collision detection
- [ ] 3D collision detection
    - [ ] binary space partitioning tree
- [ ] Rendering backend
    - [X] Vulkan
    - [ ] D3D11
    - [ ] WEBGPU
- [ ] Async task architecture (Ref: Parallelizing the Naughty Dog Engine Using Fibers)
- [ ] Audio
- [X] Unicode support

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

### GLTF skinning animation

* https://lisyarus.github.io/blog/posts/gltf-animation.html
* https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

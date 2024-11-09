#!/bin/bash
set -eu
cd "$(dirname "$0")"

# --- Unpack Arguments --------------------------------------------------------
for arg in "$@"; do declare $arg='1'; done
if [ ! -v clang ];     then gcc=1; fi
if [ ! -v release ];   then debug=1; fi
if [ -v debug ];       then echo "[debug mode]"; fi
if [ -v release ];     then echo "[release mode]"; fi
if [ -v clang ];       then compiler="${CC:-clang}"; echo "[clang compile]"; fi
if [ -v gcc ];         then compiler="${CC:-gcc}"; echo "[gcc compile]"; fi

# --- Unpack Command Line Build Arguments -------------------------------------
auto_compile_flags=''

# --- Compile/Link Line Definitions -------------------------------------------
clang_common='-I../src/ -I../local/ -g -Wno-unknown-warning-option -fdiagnostics-absolute-paths -Wall -Wno-missing-braces -Wno-unused-function -Wno-writable-strings -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Wno-initializer-overrides -Wno-incompatible-pointer-types-discards-qualifiers -Wno-for-loop-analysis -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf'
clang_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${clang_common} ${auto_compile_flags}"
clang_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${clang_common} ${auto_compile_flags}"
clang_link="-lpthread -lm -lrt -ldl -lstdc++"
clang_out="-o"
gcc_common='-I../src/ -I../local/ -g -Wno-unknown-warning-option -Wall -Wno-missing-braces -Wno-unused-function -Wno-attributes -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-compare-distinct-pointer-types -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf'
gcc_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${gcc_common} ${auto_compile_flags}"
gcc_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${gcc_common} ${auto_compile_flags}"
gcc_link="-lpthread -lm -lrt -ldl -lstdc++"
gcc_out="-o"

# --- Per-Build Settings ------------------------------------------------------
link_dll="-fPIC"
link_os_gfx="-lglfw -lvulkan -lX11 -lXext -lXxf86vm -lXrandr -lXi -lXcursor"

# --- Choose Compile/Link Lines -----------------------------------------------
if [ -v gcc ];     then compile_debug="$gcc_debug"; fi
if [ -v gcc ];     then compile_release="$gcc_release"; fi
if [ -v gcc ];     then compile_link="$gcc_link"; fi
if [ -v gcc ];     then out="$gcc_out"; fi
if [ -v clang ];   then compile_debug="$clang_debug"; fi
if [ -v clang ];   then compile_release="$clang_release"; fi
if [ -v clang ];   then compile_link="$clang_link"; fi
if [ -v clang ];   then out="$clang_out"; fi
if [ -v debug ];   then compile="$compile_debug"; fi
if [ -v release ]; then compile="$compile_release"; fi

# --- Prep Directories --------------------------------------------------------
mkdir -p build
mkdir -p local

# --- Build & Run Metaprogram -------------------------------------------------
if [ -v no_meta ]; then echo "[skipping metagen]"; fi
if [ ! -v no_meta ]
then
    cd build
    $compile_debug ../src/metagen/metagen_main.c $compile_link $out metagen
    ./metagen
    cd ..
fi

# --- Compile Shaders ---------------------------------------------------------
if [ -v no_shader ]; then echo "[skipping no_shader]"; fi
if [ ! -v no_shader ]; then
    shader_in_dir="./src/render/vulkan/shader"
    shader_out_dir="./src/render/vulkan/shader"
    for shader in "${shader_in_dir}"/*.{vert,frag}; do
      if [[ -f "$shader" ]]; then
        filename=$(basename -- "$shader")
        extension="${filename##*.}"
        name="${filename%.*}"

        # Compile the shader to the output directory with .spv extension
        echo "Compiling ${shader} to ${shader_out_dir}/${name}_${extension}.spv"
        glslc "$shader" -o "${shader_out_dir}/${name}_${extension}.spv"
      fi
    done
fi

# --- Build Everything (@build_targets) ---------------------------------------
cd build
if [ -v game ];                 then didbuild=1 && $compile ../src/game/game_main.c                                     $compile_link $link_os_gfx $out game; fi
# if [ -v raddbg ];                then didbuild=1 && $compile ../src/raddbg/raddbg_main.c                                    $compile_link $link_os_gfx $out raddbg; fi
# if [ -v rdi_from_pdb ];          then didbuild=1 && $compile ../src/rdi_from_pdb/rdi_from_pdb_main.c                        $compile_link $out rdi_from_pdb; fi
# if [ -v rdi_from_dwarf ];        then didbuild=1 && $compile ../src/rdi_from_dwarf/rdi_from_dwarf.c                         $compile_link $out rdi_from_dwarf; fi
# if [ -v rdi_dump ];              then didbuild=1 && $compile ../src/rdi_dump/rdi_dump_main.c                                $compile_link $out rdi_dump; fi
# if [ -v rdi_breakpad_from_pdb ]; then didbuild=1 && $compile ../src/rdi_breakpad_from_pdb/rdi_breakpad_from_pdb_main.c      $compile_link $out rdi_breakpad_from_pdb; fi
# if [ -v ryan_scratch ];          then didbuild=1 && $compile ../src/scratch/ryan_scratch.c                                  $compile_link $link_os_gfx $out ryan_scratch; fi
cd ..

# --- Warn On No Builds -------------------------------------------------------
if [ ! -v didbuild ]
then
  echo "[WARNING] no valid build target specified; must use build target names as arguments to this script, like \`./build.sh game\` or \`./build.sh game_mini\`."
  exit 1
fi

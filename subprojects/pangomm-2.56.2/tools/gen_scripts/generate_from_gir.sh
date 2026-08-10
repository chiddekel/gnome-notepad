#!/bin/bash

# The script assumes that it resides in the tools/gen_scripts directory and
# the XML and defs files will be placed in pango/src.

source "$(dirname "$0")/init_generate.sh"

gir_dir="$GMMPROC_GEN_INSTALL_DIR/share/gir-1.0"
out_dir="$root_dir/pango/src"

echo ===== Documentation
"$(dirname "$0")"/pango_generate_docs.sh

echo; echo ===== Enums, methods, signals and vfuncs
"$gen_with_mmgir" --gir "$gir_dir"/Pango-1.0.gir --gir "$gir_dir"/PangoCairo-1.0.gir \
  --gir-search-dir "$gir_dir" \
  --enum-defs "$out_dir"/pango_enums.defs \
  --function-defs "$out_dir"/pango_methods.defs \
  --signal-defs "$out_dir"/pango_signals.defs \
  --vfunc-defs "$out_dir"/pango_vfuncs.defs

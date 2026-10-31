#!/usr/bin/env bash
# Make the web site: draw the pictures and write the pages with site_gen, turn the pictures into WebP files in
# docs/img, and build the site with MkDocs into site/.
#
#   tools/make_site.sh           make the pictures, the pages and the site
#   tools/make_site.sh serve     the same, then serve it at http://127.0.0.1:8000
set -euo pipefail
cd "$(dirname "$0")/.."

png=${PNG_DIR:-/tmp/coriolis_site}

cmake -S . -B build > /dev/null
cmake --build build -j --target site_gen
rm -rf "$png" docs/img
./build/site_gen --docs docs --png "$png"

# Photographs of planets and screens keep more of their quality than the small pictures of the models
python3 - "$png" docs/img <<'EOF'
import os
import sys
from PIL import Image

source, target = sys.argv[1:3]
for folder, _, files in os.walk(source):
    for name in files:
        if not name.endswith(".png"):
            continue
        path = os.path.join(folder, name)
        out = os.path.join(target, os.path.relpath(path, source))[:-4] + ".webp"
        os.makedirs(os.path.dirname(out), exist_ok=True)
        quality = 85 if os.path.relpath(path, source).split(os.sep)[0] in ("planets", "screens", "flight") else 80
        Image.open(path).convert("RGB").save(out, "WEBP", quality=quality, method=6)
EOF

if [[ "${1:-}" == "serve" ]]; then
  mkdocs serve
else
  mkdocs build --strict
fi

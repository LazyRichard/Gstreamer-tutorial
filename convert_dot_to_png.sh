#!/bin/bash

# reference: https://embeddedartistry.com/blog/2018/02/22/generating-gstreamer-pipeline-graphs/
MESON_BUILD_ROOT="${MESON_BUILD_ROOT:-build}"
INPUT_DIR="${INPUT_DIR:-$MESON_BUILD_ROOT/pipeline}"

if [ -d "$1" ]; then
    DOT_FILES=`find "$1" -name "*.dot"`
    for file in $DOT_FILES
    do
        dest=`sed s/.dot/.png/ <<< "$file"`
        dot -Tpng $file > $dest
    done
else
    echo "Input directory $INPUT_DIR does not exist"
fi

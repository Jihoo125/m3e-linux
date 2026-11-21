#!/usr/bin/env bash
set -u
ROOT=/mnt/c/Users/iris8/Downloads/yes
nm -D "$ROOT/build/qml/qml/Materialized/libm3_qtplugin.so" | grep -E 'qt_plugin|MaterializedPlugin|Instance' | tail -20
head -20 "$ROOT/build/qml/Materialized/qmldir"

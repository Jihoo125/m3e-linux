set -euo pipefail
ROOT=/mnt/c/Users/iris8/Downloads/yes
cd "$ROOT"
cmake --build build/qml -j4
export QT_QPA_PLATFORM=xcb QT_QUICK_BACKEND=software QT_QUICK_CONTROLS_STYLE=Basic QML2_IMPORT_PATH="$ROOT/build/qml/qml" LIBGL_ALWAYS_SOFTWARE=1
mkdir -p build/logs
./build/qml/bin/matgallery > build/logs/gallery.log 2>&1 &
PID=$!
trap 'kill "$PID" 2>/dev/null || true' EXIT
sleep 4
if kill -0 "$PID" 2>/dev/null; then
  echo "Gallery process alive: $PID"
  import -window root build/gallery-screen.png
  kill "$PID" 2>/dev/null || true
  wait "$PID" 2>/dev/null || true
  cat build/logs/gallery.log
else
  cat build/logs/gallery.log
  exit 1
fi

#!/usr/bin/env bash
set -euo pipefail

CONFIGURATION="Release"
GENERATOR="Unix Makefiles"
CMAKE_BIN="cmake"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --config)
      CONFIGURATION="$2"
      shift 2
      ;;
    --generator)
      GENERATOR="$2"
      shift 2
      ;;
    --cmake)
      CMAKE_BIN="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VERSION_FILE="$REPO_ROOT/VERSION"
if [[ ! -f "$VERSION_FILE" ]]; then
  echo "VERSION file not found" >&2
  exit 1
fi
VERSION="$(head -n1 "$VERSION_FILE" | tr -d '\r' | xargs)"
if [[ -z "$VERSION" ]]; then
  echo "VERSION file is empty" >&2
  exit 1
fi

BUILD_DIR="$REPO_ROOT/build/linux-$CONFIGURATION"
DIST_ROOT="$REPO_ROOT/dist"
PACKAGE_NAME="AlbumManagementSystem-${VERSION}-linux-${CONFIGURATION,,}"
STAGING="$DIST_ROOT/$PACKAGE_NAME"
ARCHIVE="$DIST_ROOT/$PACKAGE_NAME.tar.gz"

mkdir -p "$BUILD_DIR"
mkdir -p "$DIST_ROOT"

"$CMAKE_BIN" -S "$REPO_ROOT" -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$CONFIGURATION"
"$CMAKE_BIN" --build "$BUILD_DIR" --config "$CONFIGURATION"

rm -rf "$STAGING"
mkdir -p "$STAGING"

BIN_PATH="$BUILD_DIR/album_management"
if [[ ! -f "$BIN_PATH" ]]; then
  BIN_PATH="$BUILD_DIR/$CONFIGURATION/album_management"
fi
if [[ ! -f "$BIN_PATH" ]]; then
  echo "Failed to locate built binary" >&2
  exit 1
fi

install "$BIN_PATH" "$STAGING/album_management"
install -m 644 "$REPO_ROOT/LICENSE" "$STAGING/"
install -m 644 "$REPO_ROOT/README.md" "$STAGING/"
install -m 644 "$REPO_ROOT/config.json" "$STAGING/" 2>/dev/null || true
install -m 644 "$REPO_ROOT/tasks.md" "$STAGING/"
install -m 644 "$REPO_ROOT/VERSION" "$STAGING/"
install -m 644 "$REPO_ROOT/version.h" "$STAGING/"
install -m 644 "$REPO_ROOT/CHANGELOG.md" "$STAGING/" 2>/dev/null || true
install -m 644 "$REPO_ROOT/RELEASE.md" "$STAGING/" 2>/dev/null || true
cp -r "$REPO_ROOT/docs" "$STAGING/"

pushd "$DIST_ROOT" >/dev/null
rm -f "$ARCHIVE"
tar -czf "$ARCHIVE" "$PACKAGE_NAME"
popd >/dev/null

echo "Created package: $ARCHIVE"
echo "Staging directory: $STAGING"

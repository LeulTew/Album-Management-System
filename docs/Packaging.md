# Packaging Guide

This guide explains how to generate distributable archives for the Album Management System using the helper scripts in `scripts/`.

---

## 1. Prerequisites

- CMake 3.10+
- Appropriate compiler toolchain (MinGW/MSVC on Windows, GCC/Clang on Linux)
- PowerShell 5.1+ for Windows script execution
- Bash (with `tar`) for Linux script execution

> Optional: delete or regenerate `.bin` data files before packaging if you want a pristine distribution.

---

## 2. Windows Packaging

1. Open **Windows PowerShell**.
2. Navigate to the repository root.
3. Run the packaging script:

```powershell
./scripts/package_windows.ps1 -Configuration Release
```

4. The script builds the project (if necessary), copies documentation, configuration, and binaries into `dist/AlbumManagementSystem-<version>-windows-Release/`, and produces a ZIP archive in the same folder.

### Customization

- Supply `-Configuration Debug` to package debug builds.
- Use `-Generator "Ninja"` (or any CMake generator) to switch toolchains.
- Provide `-CMakePath` to point to a custom CMake executable.

### Verifying Artifacts

The staging directory contains:

```
album_management.exe
config.json (optional)
LICENSE
README.md
VERSION
version.h
docs/
tasks.md
CHANGELOG.md (if present)
RELEASE.md (if present)
```

Distribute the ZIP archive from `dist/` to Windows users.

---

## 3. Linux Packaging

1. Ensure the script is executable: `chmod +x scripts/package_linux.sh`.
2. From the repository root run:

```bash
scripts/package_linux.sh --config Release
```

3. The script builds with CMake, stages artifacts under `dist/AlbumManagementSystem-<version>-linux-release/`, and compresses the result into `dist/AlbumManagementSystem-<version>-linux-release.tar.gz`.

### Flags

- `--generator` controls the CMake generator (default `"Unix Makefiles"`).
- `--cmake` lets you specify an alternative CMake binary.
- `--config Debug` outputs debug builds.

### Artifact Layout

```
album_management
config.json (optional)
LICENSE
README.md
VERSION
version.h
docs/
tasks.md
CHANGELOG.md (if present)
RELEASE.md (if present)
```

macOS users can typically consume the same tarball if built with a compatible toolchain.

---

## 4. Post-Package Checklist

- Run smoke tests from the staged directory to confirm the binary launches.
- Regenerate or delete `Artist.bin` and `Album.bin` if the packaged archive should start empty.
- Update `RELEASE.md` and `CHANGELOG.md` prior to publishing new versions.
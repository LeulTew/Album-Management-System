# Release Management Guide

1. Update the project version in `CMakeLists.txt` and the `VERSION` file.
2. Record notable changes in `CHANGELOG.md` under a new heading.
3. Regenerate `config.json` defaults if necessary.
4. Commit changes and create a Git tag using the new version number.
5. Build release binaries using the provided CMake configuration or Docker image.
6. Attach compiled artifacts and `config.json` to the GitHub release notes.

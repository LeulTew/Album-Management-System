# Album Management System — Developer Guide

This guide equips contributors with the knowledge needed to work efficiently on the Album Management System (AMS). It covers environment setup, project layout, coding standards, testing, and release workflows.

---

## 1. Environment Setup

### 1.1 Toolchain
- **Compiler**: C++17-compliant (GCC ≥ 9, Clang ≥ 10, MSVC ≥ 2019).
- **Build Systems**:
  - Code::Blocks project (`Album Management System 1.0.cbp`) for legacy workflows.
  - CMake (recommended) for cross-platform builds and CI integration.
- **Optional**: Docker (see `Dockerfile`) for reproducible builds.

### 1.2 Dependencies
The project uses only the C++ standard library. No third-party dependencies are required.

### 1.3 Repository Layout
```
├── main.cpp                # Entry point, menu orchestration
├── manager.h/.cpp          # Core business logic & repositories
├── config.json             # Default configuration (overridable)
├── tests/                  # Google Test suites (see below)
├── docs/                   # Documentation (API, user manual, developer guide)
├── scripts/                # Packaging scripts (created as part of task 8.5)
├── backups/                # Generated at runtime for snapshots
├── bin/, build/, obj/      # IDE / CMake output directories
└── VERSION                 # Source of truth for release numbering
```

---

## 2. Coding Standards

- Follow modern C++17 idioms (`auto`, range-for, smart pointers where ownership semantics demand it).
- Prefer RAII to manage resources; avoid raw `new`/`delete`.
- Keep functions focused; extract helpers when exceeding ~40 lines.
- Document complex logic with concise comments and update `docs/API_REFERENCE.md` when public surfaces change.
- Use `Logger` for noteworthy events; avoid `std::cout` in deep layers.
- Guard file I/O with `g_fileMutex` to preserve thread safety.

---

## 3. Configuration & Versioning

- Version string lives in `VERSION` and `version.h`. CMake builds populate `version.h` automatically; manual builds rely on the checked-in fallback.
- Update `CHANGELOG.md` and `RELEASE.md` alongside version bumps.
- Configuration overrides come from `config.json` (optional). When reading new keys, extend `AppConfig` to maintain defaults.

---

## 4. Testing Strategy

### 4.1 Automated Tests
- Google Test suites (`test_*.cpp`) cover managers, repositories, data integrity, and validation helpers.
- Run with your preferred build system or integrate with CTest when using CMake.

### 4.2 Manual QA
- `test_tasks.md` outlines black-box test cases. Validate UI flow after major changes.
- Smoke scripts: `run_tests.bat` and `test_suite.ps1` run automated checks + simple linting.

### 4.3 Coverage Goals
- Maintain >80% coverage on business-critical modules (managers, repositories, validation).
- Add regression tests for every bug fix touching persistence or validation.

---

## 5. Branching & Workflow

1. Fork and clone the repository.
2. Create a feature branch (`git checkout -b feature/descriptive-name`).
3. Implement changes with commits grouped logically (small, reviewable increments).
4. Update documentation (`tasks.md`, README, docs/) as part of the change.
5. Run tests locally (or through CI) before opening a pull request.
6. Request review, address feedback, and squash commits if necessary.

---

## 6. Logging & Diagnostics

- Use `Logger::getInstance()->log()` for operational events.
- `reportMemoryUsage()` dumps memory stats on shutdown (Windows + POSIX).
- Enable verbose logging when debugging file issues; instrumentation already exists in repositories.

---

## 7. Extending the System

### 7.1 Adding New Data Fields
- Extend `ArtistFile`/`AlbumFile` structs in `manager.h`.
- Update serialization helpers (`toArtistFile`, `fromArtistFile`, etc.).
- Modify UI prompts and command creation helpers.
- Adjust export/backup routines to include new fields.

### 7.2 Swapping Persistence Backend
- Implement `IArtistRepository` / `IAlbumRepository` with the new backend.
- Inject the implementation in `main.cpp` where repositories are constructed.
- Update configuration keys if connection strings or credentials are required.

### 7.3 Integrating UI Enhancements
- Menus live in `main.cpp`; keep them thin by delegating heavy logic to managers.
- For progress indicators, reuse the existing console formatting utilities.

---

## 8. Release Process

1. Update `VERSION`, `CHANGELOG.md`, and `RELEASE.md`.
2. Regenerate `version.h` if not using the CMake pipeline.
3. Run full test suite (unit + manual smoke).
4. Use packaging scripts in `scripts/` to produce platform-specific archives.
5. Upload artifacts to GitHub Releases or the appropriate distribution channel.

---

## 9. Support & Further Reading

- [`docs/API_REFERENCE.md`](API_REFERENCE.md) — API surface area.
- [`docs/UserManual.md`](UserManual.md) — UI/operations.
- [`README.md`](../README.md) — high-level overview and quick start.
- [`tasks.md`](../tasks.md) — improvement tracker with completion status.

Happy hacking! Contributions are always welcome—open a discussion or issue if you plan a large refactor.

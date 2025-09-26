# Album Management System API Reference

This document collects the public-facing APIs that external contributors are expected to use when extending or embedding the Album Management System. The project follows a layered architecture with **Domain Models**, **Managers**, **Repository Interfaces**, **Command Infrastructure**, and **Support Utilities**. All interfaces are implemented in standard C++17.

> ℹ️  Namespaces: the codebase currently resides in the global namespace. When extending the system, prefer keeping new symbols inside a dedicated namespace to avoid collisions.

---

## Domain Models

### `class Artist`
Represents a musical artist with identity metadata.

| Method | Description |
| --- | --- |
| `void setArtistId(std::string id)` / `std::string getArtistId() const` | Assign or retrieve the canonical artist ID (format: `art####`). |
| `void setName(std::string name)` / `std::string getName() const` | Manage the artist's display name. Validation enforces printable characters. |
| `void setGender(char gender)` / `char getGender() const` | Store gender flag (`'M'`, `'F'`, `'N'` neutral). |
| `void setPhone(std::string phone)` / `std::string getPhone() const` | Persist E.164-style phone numbers. |
| `void setEmail(std::string email)` / `std::string getEmail() const` | Persist validated e-mail addresses. |

### `class Album`
Represents a single album that belongs to an `Artist`.

| Method | Description |
| --- | --- |
| `void setAlbumId(std::string id)` / `std::string getAlbumId() const` | Assign canonical album ID (`alb####`). |
| `void setArtistId(std::string artistId)` / `std::string getArtistId() const` | Link album to artist. |
| `void setTitle(std::string title)` / `std::string getTitle() const` | Manage album title (UTF-8 aware). |
| `void setRecordFormat(std::string format)` / `std::string getRecordFormat() const` | Store original format (CD, Vinyl, Digital…). |
| `void setDatePublished(std::string date)` / `std::string getDatePublished() const` | Persist publishing date (`DD/MM/YYYY`). |
| `void setPath(std::string path)` / `std::string getPath() const` | Associate album asset path or URI. |

---

## Manager Layer

### `class ArtistManager`
Coordinates CRUD operations, validation, and UI interactions for artists.

Key methods:
- `bool addArtist(std::fstream& artistFile, artistList& cache)` — Validates user input, assigns IDs, and queues a command.
- `bool editArtist(...)` — Wraps updates in undoable commands.
- `bool removeArtist(...)` — Deletes artist and cascading albums through the command framework.
- `void viewArtist(...)` — Displays artist detail pages using formatted console output.

### `class AlbumManager`
Equivalent orchestration for albums with helpers to filter by artist, validate record formats, and update binary storage.

---

## Command Infrastructure

### `struct CommandAction`
Represents a redo/undo-capable unit of work.

| Member | Description |
| --- | --- |
| `std::function<bool()> redo` | Executed when the command is dispatched; returning `false` aborts the flow. |
| `std::function<void()> undo` | Reverts the operation. |
| `std::string description` | Human readable message used in logs/UI. |

### `class CommandManager`
Maintains undo/redo stacks.

| Method | Description |
| --- | --- |
| `bool execute(CommandAction action)` | Invokes `redo`, pushes onto undo stack, clears redo history. |
| `bool undo()` | Pops top undo command; invokes its `undo` lambda and pushes to redo stack. |
| `bool redo()` | Replays the most recent undone command. |
| `bool canUndo() const` / `bool canRedo() const` | Query stack availability for UI hints. |
| `void clear()` | Empties both stacks (used after destructive operations such as restores). |

> ⚠️  Thread safety: the manager itself is guarded by higher-level mutexes when interacting with file operations. Prefer pushing commands that interact with the repository layer through existing helpers.

---

## Repository Interfaces

### `class IArtistRepository`
Abstract interface (see `manager.h`).

| Method | Notes |
| --- | --- |
| `virtual bool loadArtists(artistList&, indexSet&) = 0` | Load fixed-width binary records into memory and gather deleted slots. |
| `virtual bool saveArtist(const Artist&) = 0` | Append new record. |
| `virtual bool updateArtist(const Artist&, int position) = 0` | Overwrite record at byte-offset `position`. |
| `virtual bool deleteArtist(int position) = 0` | Write sentinel record (`-1`) at position. |
| `virtual bool saveArtists(const artistList&, const indexSet&) = 0` | Rewrite entire store from cache. |
| `virtual ~IArtistRepository() = default` | Virtual destructor. |

### `class FileArtistRepository : public IArtistRepository`
Binary-file implementation.

- Uses `openFile` helper to create/lock `Artist.bin`.
- All read/write paths guard access with the global `g_fileMutex`.
- Logs failures via `Logger` and surfaces them to callers.

### `class IAlbumRepository` / `class FileAlbumRepository`
Mirrors the artist repository with album-specific structs (`AlbumFile`). Additional methods support batch saves and search scaffolding.

---

## Support Utilities

### `class Logger`
Thread-safe singleton writing to `album_system.log`.

| Method | Description |
| --- | --- |
| `static Logger* getInstance()` | Lazily creates the logger. |
| `void log(const std::string& message)` | Appends timestamped entry. |
| `void flush()` | Forces filesystem flush (used during shutdown). |

### Configuration Helpers

| Function | Description |
| --- | --- |
| `void loadApplicationConfig(const std::string& path)` | Parses `config.json` and applies overrides for file locations. |
| `const std::string& getArtistFilePath()` | Accessor for configured artist binary path. |
| `const std::string& getAlbumFilePath()` | Accessor for album binary path. |
| `const std::string& getBackupDirectory()` | Directory containing snapshots. |
| `const std::string& getBackupIndexFile()` | Path to backup index CSV. |

### Backup & Integrity Tools

| Function | Description |
| --- | --- |
| `bool backupData(...)` | Captures timestamped backups, computes checksums, and appends to index. |
| `bool restoreFromBackup(...)` | Validates checksum, restores `.bin` files, reloads caches. |
| `std::string computeFileChecksum(const std::string& path)` | Uses FNV-1a hashing for integrity verification. |

> ✅  Every low-level file helper (`readArtistAtPosition`, `appendAlbumRecord`, etc.) is now guarded by `std::recursive_mutex g_fileMutex` ensuring thread-safe concurrent operations.

---

## Error Handling Contract

- Most repository and helper functions return `bool`. `false` indicates an error; callers log and surface friendly messages to the UI.
- Fatal file creation/opening issues throw `FileException` (derived from `std::runtime_error`).
- Validation helpers throw `ValidationException` with descriptive messages when user inputs break invariants.
- Command undo/redo functions are expected to be exception-safe; wrap complex logic with `try/catch` if needed to keep stacks consistent.

---

## Extending the API

1. **Add new repositories** by implementing the relevant interface; register them with managers via dependency injection points in `main.cpp` or configuration.
2. **Introduce new commands** by constructing `CommandAction` objects that encapsulate redo/undo behavior and push them through `CommandManager`.
3. **Expose new features** in the UI by updating menu handlers (see `main.cpp`) and reusing validation + logging utilities.
4. **Document changes** by updating this reference and the developer guide so future contributors understand the extended surface area.

---

## Related Documents

- [`docs/UserManual.md`](UserManual.md) — end-user navigation and workflows.
- [`docs/DeveloperGuide.md`](DeveloperGuide.md) — setup, architecture, and extension practices.
- [`README.md`](../README.md) — project overview + quick start.

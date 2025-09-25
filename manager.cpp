/**
 * @file manager.cpp
 * @brief Implementation file for the Album Management System.
 *
 * This file contains the implementation of all functions declared in manager.h.
 * It includes the main application logic, file operations, user interface handling,
 * data validation, and business logic for managing artists and albums.
 */

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include <algorithm>
#include <functional>
#include <stack>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <chrono>
#include <cerrno>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <Windows.h>
#endif
#include "manager.h"

using namespace std;

int lastArtistID = 999, lastAlbumID = 1999;
Logger* Logger::instance = nullptr;

struct CommandAction {
    std::function<bool()> redo;
    std::function<void()> undo;
    std::string description;
};

class CommandManager {
private:
    std::stack<CommandAction> undoStack;
    std::stack<CommandAction> redoStack;

public:
    bool execute(CommandAction action) {
        if (action.redo && action.redo()) {
            undoStack.push(action);
            // Clear redo history after a new operation
            while (!redoStack.empty()) {
                redoStack.pop();
            }
            return true;
        }
        return false;
    }

    bool undo() {
        if (undoStack.empty()) {
            return false;
        }
        CommandAction action = undoStack.top();
        undoStack.pop();
        if (action.undo) {
            action.undo();
        }
        redoStack.push(action);
        return true;
    }

    bool redo() {
        if (redoStack.empty()) {
            return false;
        }
        CommandAction action = redoStack.top();
        redoStack.pop();
        if (action.redo && action.redo()) {
            undoStack.push(action);
            return true;
        }
        return false;
    }

    bool canUndo() const {
        return !undoStack.empty();
    }

    bool canRedo() const {
        return !redoStack.empty();
    }

    std::string nextUndoDescription() const {
        if (undoStack.empty()) {
            return "";
        }
        return undoStack.top().description;
    }

    std::string nextRedoDescription() const {
        if (redoStack.empty()) {
            return "";
        }
        return redoStack.top().description;
    }

    void clear() {
        while (!undoStack.empty()) undoStack.pop();
        while (!redoStack.empty()) redoStack.pop();
    }
};

static CommandManager commandManager;

namespace {
#ifdef _WIN32
const char PATH_SEPARATOR = '\\';
#else
const char PATH_SEPARATOR = '/';
#endif

struct BackupEntry {
    std::string timestamp;
    std::string artistFile;
    std::string albumFile;
};

std::string joinPath(const std::string& dir, const std::string& file) {
    if (dir.empty()) {
        return file;
    }
    if (dir.back() == '/' || dir.back() == '\\') {
        return dir + file;
    }
    return dir + PATH_SEPARATOR + file;
}

bool ensureDirectoryExists(const std::string& dir) {
#ifdef _WIN32
    if (_mkdir(dir.c_str()) == 0) {
        return true;
    }
#else
    if (mkdir(dir.c_str(), 0755) == 0) {
        return true;
    }
#endif
    if (errno == EEXIST) {
        return true;
    }
    return false;
}

bool ensureIndexFileExists() {
    if (!ensureDirectoryExists(backupDirectory)) {
        return false;
    }
    std::ifstream check(backupIndexFile);
    if (check.good()) {
        return true;
    }
    check.close();
    std::ofstream create(backupIndexFile, std::ios::app);
    return create.good();
}

bool fileExists(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return file.good();
}

bool copyFile(const std::string& source, const std::string& destination) {
    std::ifstream src(source, std::ios::binary);
    if (!src) {
        return false;
    }
    std::ofstream dst(destination, std::ios::binary);
    if (!dst) {
        return false;
    }
    dst << src.rdbuf();
    return dst.good();
}

bool copyFileOverwrite(const std::string& source, const std::string& destination) {
    auto* logger = Logger::getInstance();

    // Verify the source exists before we begin any overwrite attempts.
    std::ifstream sourceCheck(source, std::ios::binary);
    if (!sourceCheck) {
        logger->log("copyFileOverwrite: Failed to open source file: " + source);
        return false;
    }
    sourceCheck.close();

    const int directCopyRetries = 5;
    const int fallbackRetries = 10;
    const int retryDelayMs = 150;

    // First try to overwrite in-place using truncation, which avoids deleting the file.
    for (int attempt = 0; attempt < directCopyRetries; ++attempt) {
        std::ifstream src(source, std::ios::binary);
        if (!src) {
            logger->log("copyFileOverwrite: Source became unavailable before attempt " + std::to_string(attempt + 1));
            return false;
        }

        errno = 0;
        std::ofstream dst(destination, std::ios::binary | std::ios::trunc);
        int openErrno = errno;
        if (!dst) {
            logger->log("copyFileOverwrite: Failed to open destination for overwrite on attempt " +
                        std::to_string(attempt + 1) + ", errno: " + std::to_string(openErrno));
        } else {
            dst << src.rdbuf();
            dst.flush();

            if (dst && dst.good()) {
                logger->log("copyFileOverwrite: Overwrote destination via truncation on attempt " + std::to_string(attempt + 1));
                return true;
            }

            logger->log("copyFileOverwrite: Failed to stream data to destination on attempt " +
                        std::to_string(attempt + 1) + ", errno: " + std::to_string(errno));
        }

#ifdef _WIN32
        Sleep(retryDelayMs);
#else
        usleep(retryDelayMs * 1000);
#endif
    }

    // Fallback to copy into a temporary file and then replace the destination.
    std::ifstream src(source, std::ios::binary);
    if (!src) {
        logger->log("copyFileOverwrite: Failed to open source during fallback copy: " + source);
        return false;
    }

    std::string tempDest = destination + ".tmp";
    std::ofstream dst(tempDest, std::ios::binary);
    if (!dst) {
        logger->log("copyFileOverwrite: Failed to create temp file: " + tempDest);
        return false;
    }

    dst << src.rdbuf();
    dst.flush();
    if (!dst.good()) {
        dst.close();
        std::remove(tempDest.c_str());
        logger->log("copyFileOverwrite: Failed to write to temp file: " + tempDest);
        return false;
    }
    dst.close();
    src.close();

    for (int attempt = 0; attempt < fallbackRetries; ++attempt) {
#ifdef _WIN32
        SetFileAttributesA(destination.c_str(), FILE_ATTRIBUTE_NORMAL);
#endif

        errno = 0;
        if (std::remove(destination.c_str()) == 0 || errno == ENOENT) {
            errno = 0;
            if (std::rename(tempDest.c_str(), destination.c_str()) == 0) {
                logger->log("copyFileOverwrite: Successfully copied " + source + " to " + destination +
                            " on fallback attempt " + std::to_string(attempt + 1));
                return true;
            }

            int renameErrno = errno;
#ifdef _WIN32
            DWORD lastError = GetLastError();
            logger->log("copyFileOverwrite: Failed to rename temp file on fallback attempt " +
                        std::to_string(attempt + 1) + ", errno: " + std::to_string(renameErrno) +
                        ", lastError: " + std::to_string(lastError));
#else
            logger->log("copyFileOverwrite: Failed to rename temp file on fallback attempt " +
                        std::to_string(attempt + 1) + ", errno: " + std::to_string(renameErrno));
#endif
        } else {
            int removeErrno = errno;
            logger->log("copyFileOverwrite: Failed to remove destination file on fallback attempt " +
                        std::to_string(attempt + 1) + ", errno: " + std::to_string(removeErrno));
        }

#ifdef _WIN32
        Sleep(retryDelayMs);
#else
        usleep(retryDelayMs * 1000);
#endif
    }

    std::remove(tempDest.c_str());
    logger->log("copyFileOverwrite: All retries failed for copying " + source + " to " + destination);
    return false;
}

std::string makeTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

bool appendBackupEntry(const BackupEntry& entry) {
    if (!ensureIndexFileExists()) {
        return false;
    }
    std::ofstream indexFile(backupIndexFile, std::ios::app);
    if (!indexFile) {
        return false;
    }
    indexFile << entry.timestamp << ',' << entry.artistFile << ',' << entry.albumFile << '\n';
    return indexFile.good();
}

std::vector<BackupEntry> loadBackupEntries() {
    std::vector<BackupEntry> entries;
    std::ifstream indexFile(backupIndexFile);
    if (!indexFile) {
        return entries;
    }
    std::string line;
    while (std::getline(indexFile, line)) {
        if (line.empty()) {
            continue;
        }
        std::stringstream ss(line);
        BackupEntry entry;
        if (!std::getline(ss, entry.timestamp, ',')) {
            continue;
        }
        if (!std::getline(ss, entry.artistFile, ',')) {
            continue;
        }
        if (!std::getline(ss, entry.albumFile, ',')) {
            continue;
        }
        entries.push_back(entry);
    }
    std::sort(entries.begin(), entries.end(), [](const BackupEntry& a, const BackupEntry& b) {
        return a.timestamp > b.timestamp;
    });
    return entries;
}

void displayBackupEntries(const std::vector<BackupEntry>& entries) {
    cout << "\nAvailable backups: \n";
    cout << left << setw(6) << "[#]" << setw(25) << "Timestamp" << "Snapshot Files" << endl;
    cout << setw(6) << "---" << setw(25) << "-----------------------" << "---------------------------------------" << endl;
    for (size_t i = 0; i < entries.size(); ++i) {
        cout << setw(6) << (i + 1)
             << setw(25) << entries[i].timestamp
             << entries[i].artistFile << " | " << entries[i].albumFile << endl;
    }
    cout << endl;
}

} // namespace

static bool createBackupSnapshot(std::fstream& ArtFile, std::fstream& AlbFile) {
    cout << "Creating backup snapshot..." << endl;
    ArtFile.flush();
    AlbFile.flush();
    ArtFile.clear();
    AlbFile.clear();

    std::string timestamp = makeTimestamp();
    std::string artistFileName = "Artist_" + timestamp + ".bin";
    std::string albumFileName = "Album_" + timestamp + ".bin";
    std::string artistBackupPath = joinPath(backupDirectory, artistFileName);
    std::string albumBackupPath = joinPath(backupDirectory, albumFileName);

    if (!ensureIndexFileExists()) {
        cout << "Failed to prepare backup directory." << endl;
        Logger::getInstance()->log("Backup failed: unable to prepare directory");
        system("pause");
        return false;
    }

    if (!fileExists(artistFilePath) || !fileExists(albumFilePath)) {
        cout << "Cannot create backup: source data files missing." << endl;
        Logger::getInstance()->log("Backup failed: source files missing");
        system("pause");
        return false;
    }

    if (!copyFile(artistFilePath, artistBackupPath)) {
        cout << "Failed to backup artist data." << endl;
        Logger::getInstance()->log("Backup failed: unable to copy artist file");
        system("pause");
        return false;
    }

    if (!copyFile(albumFilePath, albumBackupPath)) {
        std::remove(artistBackupPath.c_str());
        cout << "Failed to backup album data." << endl;
        Logger::getInstance()->log("Backup failed: unable to copy album file");
        system("pause");
        return false;
    }

    BackupEntry entry{timestamp, artistFileName, albumFileName};
    if (!appendBackupEntry(entry)) {
        cout << "Backup created, but failed to update index." << endl;
        Logger::getInstance()->log("Backup warning: unable to append index entry");
    } else {
        Logger::getInstance()->log("Backup created: " + timestamp);
    }

    cout << "Backup snapshot saved as:\n  " << artistBackupPath << "\n  " << albumBackupPath << endl;
    system("pause");
    return true;
}

static bool restoreFromBackup(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtArray, indexSet& delAlbArray) {
    cout << "Restoring from backup..." << endl;
    auto entries = loadBackupEntries();
    if (entries.empty()) {
        cout << "No backups found. Create one first." << endl;
        system("pause");
        return false;
    }

    displayBackupEntries(entries);
    int selection = -1;
    while (true) {
        cout << "Select backup to restore (0 to cancel): ";
        if (!(cin >> selection)) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid input. Please enter a number." << endl;
            continue;
        }
        cin.ignore(INT_MAX, '\n');
        if (selection == 0) {
            cout << "Restore cancelled." << endl;
            system("pause");
            return false;
        }
        if (selection < 1 || selection > static_cast<int>(entries.size())) {
            cout << "Invalid choice. Try again." << endl;
            continue;
        }
        break;
    }

    const BackupEntry& chosen = entries[selection - 1];
    std::string artistBackupPath = joinPath(backupDirectory, chosen.artistFile);
    std::string albumBackupPath = joinPath(backupDirectory, chosen.albumFile);

    if (!fileExists(artistBackupPath) || !fileExists(albumBackupPath)) {
        cout << "Backup files missing on disk. Operation aborted." << endl;
        Logger::getInstance()->log("Restore failed: missing backup files for " + chosen.timestamp);
        system("pause");
        return false;
    }

    char confirm;
    cout << "Restoring will overwrite current data files. Continue? (Y/N): ";
    if (!(cin >> confirm)) {
        cin.clear();
        cin.ignore(INT_MAX, '\n');
        cout << "Restore cancelled." << endl;
        system("pause");
        return false;
    }
    cin.ignore(INT_MAX, '\n');
    if (confirm != 'y' && confirm != 'Y') {
        cout << "Restore cancelled." << endl;
        system("pause");
        return false;
    }

    ArtFile.flush();
    AlbFile.flush();
    ArtFile.close();
    AlbFile.close();

    // Add delay to ensure file handles are released on Windows
#ifdef _WIN32
    Sleep(500);  // 500ms delay
#else
    usleep(500000);  // 500ms delay
#endif

    if (!copyFileOverwrite(artistBackupPath, artistFilePath) || !copyFileOverwrite(albumBackupPath, albumFilePath)) {
        cout << "Failed to restore backup." << endl;
        Logger::getInstance()->log("Restore failed while copying backup snapshot " + chosen.timestamp);
        try {
            openFile(ArtFile, artistFilePath);
            openFile(AlbFile, albumFilePath);
        } catch (...) {
            // Ignore: openFile already reports errors elsewhere.
        }
        system("pause");
        return false;
    }

    artist.artList.clear();
    album.albList.clear();
    delArtArray.indexes.clear();
    delAlbArray.indexes.clear();
    lastArtistID = 999;
    lastAlbumID = 1999;

    if (!loadArtist(ArtFile, artist, delArtArray) || !loadAlbum(AlbFile, album, delAlbArray)) {
        cout << "Backup restored, but failed to reload data into memory." << endl;
        Logger::getInstance()->log("Restore warning: reload failed for snapshot " + chosen.timestamp);
        system("pause");
        return false;
    }

    commandManager.clear();
    Logger::getInstance()->log("Restore completed from snapshot " + chosen.timestamp);
    cout << "Restore completed successfully." << endl;
    system("pause");
    return true;
}

void backupAndRestoreMenu(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtArray, indexSet& delAlbArray) {
    bool exitMenu = false;
    do {
        int choice = MenuView::backupMenu();
        switch (choice) {
            case 1:
                createBackupSnapshot(ArtFile, AlbFile);
                break;
            case 2:
                restoreFromBackup(ArtFile, AlbFile, artist, album, delArtArray, delAlbArray);
                break;
            case 3:
                exitMenu = true;
                break;
            default:
                break;
        }
    } while (!exitMenu);
}

bool undoLastAction() {
    return commandManager.undo();
}

bool redoLastAction() {
    return commandManager.redo();
}

bool canUndo() {
    return commandManager.canUndo();
}

bool canRedo() {
    return commandManager.canRedo();
}

std::string getNextUndoDescription() {
    return commandManager.nextUndoDescription();
}

std::string getNextRedoDescription() {
    return commandManager.nextRedoDescription();
}

static bool executeCommand(CommandAction action) {
    return commandManager.execute(std::move(action));
}

struct ArtistCommandState {
    Artist artist;
    long pos = -1;
};

struct ArtistEditState {
    Artist original;
    Artist updated;
    long pos = -1;
    bool applied = false;
};

struct AlbumCommandState {
    Album album;
    long pos = -1;
};

struct AlbumEditState {
    Album original;
    Album updated;
    long pos = -1;
    bool applied = false;
};

struct AlbumSnapshot {
    Album data;
    long pos = -1;
};

struct ArtistRemovalState {
    Artist artist;
    long pos = -1;
    int artistIndex = -1;
    std::vector<AlbumSnapshot> associatedAlbums;
};

struct AlbumRemovalState {
    Album album;
    long pos = -1;
    int index = -1;
};

static bool ensureArtistStream(std::fstream& ArtFile) {
    if (ArtFile.is_open()) {
        return true;
    }
    try {
        openFile(ArtFile, artistFilePath);
        return true;
    } catch (const FileException& e) {
        cout << e.what() << endl;
        system("pause");
        return false;
    }
}

static bool ensureAlbumStream(std::fstream& AlbFile) {
    if (AlbFile.is_open()) {
        return true;
    }
    try {
        openFile(AlbFile, albumFilePath);
        return true;
    } catch (const FileException& e) {
        cout << e.what() << endl;
        system("pause");
        return false;
    }
}

static ArtistFile toArtistFile(const Artist& artist) {
    ArtistFile artFile{};
    strncpy(artFile.artistIds, artist.getArtistId().c_str(), 7);
    artFile.artistIds[7] = '\0';
    strncpy(artFile.names, artist.getName().c_str(), 49);
    artFile.names[49] = '\0';
    artFile.genders = artist.getGender();
    strncpy(artFile.phones, artist.getPhone().c_str(), 14);
    artFile.phones[14] = '\0';
    strncpy(artFile.emails, artist.getEmail().c_str(), 49);
    artFile.emails[49] = '\0';
    return artFile;
}

static Artist fromArtistFile(const ArtistFile& artFile) {
    Artist artist;
    artist.setArtistId(std::string(artFile.artistIds));
    artist.setName(std::string(artFile.names));
    artist.setGender(artFile.genders);
    artist.setPhone(std::string(artFile.phones));
    artist.setEmail(std::string(artFile.emails));
    return artist;
}

static AlbumFile toAlbumFile(const Album& album) {
    AlbumFile albFile{};
    strncpy(albFile.albumIds, album.getAlbumId().c_str(), 7);
    albFile.albumIds[7] = '\0';
    strncpy(albFile.artistIdRefs, album.getArtistId().c_str(), 7);
    albFile.artistIdRefs[7] = '\0';
    strncpy(albFile.titles, album.getTitle().c_str(), 79);
    albFile.titles[79] = '\0';
    strncpy(albFile.recordFormats, album.getRecordFormat().c_str(), 11);
    albFile.recordFormats[11] = '\0';
    strncpy(albFile.datePublished, album.getDatePublished().c_str(), 10);
    albFile.datePublished[10] = '\0';
    strncpy(albFile.paths, album.getPath().c_str(), 99);
    albFile.paths[99] = '\0';
    return albFile;
}

static Album fromAlbumFile(const AlbumFile& albFile) {
    Album album;
    album.setAlbumId(std::string(albFile.albumIds));
    album.setArtistId(std::string(albFile.artistIdRefs));
    album.setTitle(std::string(albFile.titles));
    album.setRecordFormat(std::string(albFile.recordFormats));
    album.setDatePublished(std::string(albFile.datePublished));
    album.setPath(std::string(albFile.paths));
    return album;
}

static bool readArtistAtPosition(std::fstream& ArtFile, long pos, Artist& artist) {
    if (!ensureArtistStream(ArtFile)) {
        return false;
    }
    ArtistFile artFile{};
    ArtFile.clear();
    ArtFile.seekg(pos, std::ios::beg);
    if (!ArtFile.read(reinterpret_cast<char*>(&artFile), sizeof(ArtistFile))) {
        return false;
    }
    artist = fromArtistFile(artFile);
    return true;
}

static bool writeArtistAtPosition(std::fstream& ArtFile, long pos, const Artist& artist) {
    if (!ensureArtistStream(ArtFile)) {
        return false;
    }
    ArtistFile artFile = toArtistFile(artist);
    ArtFile.clear();
    ArtFile.seekp(pos, std::ios::beg);
    ArtFile.write(reinterpret_cast<const char*>(&artFile), sizeof(ArtistFile));
    ArtFile.flush();
    return static_cast<bool>(ArtFile);
}

static bool appendArtistRecord(std::fstream& ArtFile, const Artist& artist, long& outPos) {
    if (!ensureArtistStream(ArtFile)) {
        return false;
    }
    ArtistFile artFile = toArtistFile(artist);
    ArtFile.clear();
    ArtFile.seekp(0, std::ios::end);
    outPos = ArtFile.tellp();
    ArtFile.write(reinterpret_cast<const char*>(&artFile), sizeof(ArtistFile));
    ArtFile.flush();
    return static_cast<bool>(ArtFile);
}

static bool readAlbumAtPosition(std::fstream& AlbFile, long pos, Album& album) {
    if (!ensureAlbumStream(AlbFile)) {
        return false;
    }
    AlbumFile albFile{};
    AlbFile.clear();
    AlbFile.seekg(pos, std::ios::beg);
    if (!AlbFile.read(reinterpret_cast<char*>(&albFile), sizeof(AlbumFile))) {
        return false;
    }
    album = fromAlbumFile(albFile);
    return true;
}

static bool writeAlbumAtPosition(std::fstream& AlbFile, long pos, const Album& album) {
    if (!ensureAlbumStream(AlbFile)) {
        return false;
    }
    AlbumFile albFile = toAlbumFile(album);
    AlbFile.clear();
    AlbFile.seekp(pos, std::ios::beg);
    AlbFile.write(reinterpret_cast<const char*>(&albFile), sizeof(AlbumFile));
    AlbFile.flush();
    return static_cast<bool>(AlbFile);
}

static bool appendAlbumRecord(std::fstream& AlbFile, const Album& album, long& outPos) {
    if (!ensureAlbumStream(AlbFile)) {
        return false;
    }
    AlbumFile albFile = toAlbumFile(album);
    AlbFile.clear();
    AlbFile.seekp(0, std::ios::end);
    outPos = AlbFile.tellp();
    AlbFile.write(reinterpret_cast<const char*>(&albFile), sizeof(AlbumFile));
    AlbFile.flush();
    return static_cast<bool>(AlbFile);
}

static int findArtistIndexById(const artistList& artists, const std::string& artistId) {
    for (size_t i = 0; i < artists.artList.size(); ++i) {
        if (artists.artList[i].artistId == artistId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

static int findAlbumIndexById(const albumList& albums, const std::string& albumId) {
    for (size_t i = 0; i < albums.albList.size(); ++i) {
        if (albums.albList[i].albumId == albumId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/**
 * @brief Displays the welcome message to the user.
 */
void welcome()  //2
{
    system("COLOR 2F");
	cout<<"\n\n";
	cout<<setfill('~')<<setw(150)<<'~';
	cout<<"\n";
	cout<<"\n                          |o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o|     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |                    WELCOME                      |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |              o ALBUM MANAGEMENT o               |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |             Enter ENTER to continue...          |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o|     ";
	cout<<'\n'<<setw(150)<<'~'<<setfill(' ');
	cout<<endl<<endl;
    system("pause");
}

//
void printError(int errId){
    switch (errId){
        case 1:
            cout << "\t\n Error~ Artist file can not be opened!" << endl;
            break;
        case 2:
            cout << "\t\n Error~ Album file can not be opened!" << endl;
            break;
        case 3:
            cout << "\t\n Error~No sufficient memory. Program can not continue!" << endl;
            break;
        case 4:
            cout << "\t\n Error~no search results found.\n" << endl;
            break;
        case 5:
            cout << " \t\n Error~ No sufficient memory to create a space for the result array!" << endl;
            break;
    }
}

//
void intToCharArray(int last, char id[], char prefix[])
{
    int i=6;
    strcpy(id, prefix);
    while(i>2){
        id[i--] = last%10 + '0';
        last /= 10;
    }
}

//
int charArrayToInt(char *arr)
{
    int value, flag, r;
    flag = 1;
    value = 0;
    for( size_t i=3; i<strlen(arr); i++){
        if( i==0 && arr[i]=='-' ){
            flag = -1;
            continue;
        }
        r = arr[i] - '0';
        value = value * 10 + r;
    }
    value = value * flag;
    return value;
}

//
void openFile(std::fstream& fstr, const std::string& path)
{
    cout<<'\n';
    
    // Try to open file for reading and writing
    fstr.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!fstr) {
        // File doesn't exist, create it
        std::ofstream createFile(path, std::ios::binary);
        if (!createFile) {
            throw FileException("Failed to create file: " + path);
        }
        createFile.close();
        
        // Now open it for reading and writing
        fstr.open(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!fstr) {
            throw FileException("Failed to open file: " + path);
        }
    }
}

std::string intToString(int last, const std::string& prefix) {
    return prefix + std::to_string(last);
}

int stringToInt(const std::string& arr) {
    // Assuming format is prefix + number, extract the number part
    size_t start = 0;
    while (start < arr.size() && !isdigit(arr[start])) start++;
    if (start == arr.size()) return 0;
    return std::stoi(arr.substr(start));
}

//3
bool loading(fstream & ArtFile, fstream & AlbFile, artistList & artist, albumList & album, indexSet & delArtFile, indexSet & delAlbFile)
{
    int i = 0;
 	char load[26];
 	while(i < 25)
 	{
 		system("cls");
 		load[i++] = '#';
 		load[i] = '\0';
        cout<<"\n\n\n\n\n\n\n\t\t\t\tLOADING: "<< load;
 		usleep(199900);
 	}
    system("clear");
    cout<<"\n";
    system("cls");

    // Check if files are empty and create sample data if needed
    if (!loadArtist(ArtFile, artist, delArtFile) || artist.artList.empty()) {
        // createSampleData(ArtFile, AlbFile, artist, album);
        // Reload after creating sample data
        // artist.artList.clear();
        // album.albList.clear();
        // loadArtist(ArtFile, artist, delArtFile);
    }

    if(!loadAlbum(AlbFile, album, delAlbFile))
        return false;

    return true;
}

//5
bool loadArtist(std::fstream& ArtFile, artistList& artist, indexSet& delArtFile)
{
    ArtistFile artFile;
    int nRec, pos;
    std::string id;

    try {
        openFile(ArtFile, artistFilePath);
    } catch(const FileException& e) {
        printError(1);
        system("pause");
        return false;
    }
    ArtFile.seekg(0, ios::end);
    nRec = ArtFile.tellg() / sizeof(artFile);
    artist.artList.reserve(nRec + DEFAULT_SIZE);
    ArtFile.seekg(0, ios::beg);
    pos = 0;
    for (int i = 0; i < nRec; i++){
        ArtFile.read((char*)&artFile, sizeof(artFile));
        artFile.artistIds[7] = '\0';
        artFile.names[49] = '\0';
        if(std::string(artFile.artistIds) != "-1"){
            artist.artList.push_back({std::string(artFile.artistIds), std::string(artFile.names), pos});
            int currentId = stringToInt(std::string(artFile.artistIds));
            if(currentId > lastArtistID){
                lastArtistID = currentId;
            }
        }
        else{
            delArtFile.indexes.push_back(pos);
        }
        pos = ArtFile.tellg();
    }
    sortArtist(artist);
    return true;
}

//6
bool loadAlbum(std::fstream& AlbFile, albumList& album, indexSet& delAlbFile)
{
    AlbumFile albFile;
    int nRec, pos;

    try {
        openFile(AlbFile, albumFilePath);
    } catch(const FileException& e) {
        printError(2);
        system("pause");
        return false;
    }
    AlbFile.seekg(0, ios::end);
    nRec = AlbFile.tellg() / sizeof(albFile);
    album.albList.reserve(nRec + DEFAULT_SIZE);
    AlbFile.seekg(0, ios::beg);
    pos = 0;
    for (int i = 0; i < nRec; i++){
        AlbFile.read((char*)&albFile, sizeof(albFile));
        albFile.albumIds[7] = '\0';
        albFile.artistIdRefs[7] = '\0';
        albFile.titles[79] = '\0';
        if (std::string(albFile.albumIds) != "-1"){
            album.albList.push_back(albumIndex{std::string(albFile.albumIds), std::string(albFile.artistIdRefs), std::string(albFile.titles), pos});
            int currentId = stringToInt(std::string(albFile.albumIds));
            if (currentId > lastAlbumID){
                lastAlbumID = currentId;
            }
        }
        else{
            delAlbFile.indexes.push_back(pos);
        }
        pos = AlbFile.tellg();
    }
    sortAlbum(album);
    return true;
}

//7
void sortArtist(artistList& artist)
{
    std::sort(artist.artList.begin(), artist.artList.end(), [](const artistIndex& a, const artistIndex& b) {
        return a.name < b.name;
    });
}

//8
void sortAlbum(albumList& album)
{
    std::sort(album.albList.begin(), album.albList.end(), [](const albumIndex& a, const albumIndex& b) {
        return a.artistId < b.artistId;
    });
}

//9
void mainH(fstream & ArtFile, fstream & AlbFile, artistList & artist, albumList & album, indexSet & result, indexSet & delArtArray, indexSet & delAlbArray)
{
    bool exit=false;
    do
    {
        int choice=MenuView::mainMenu();
        switch (choice) {
            case 1:
                exit = artistManager(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
                break;
            case 2:
                exit = albumManager(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
                break;
            case 3:
                backupAndRestoreMenu(ArtFile, AlbFile, artist, album, delArtArray, delAlbArray);
                exit = false;
                break;
            case 4:
                displayStatistics(artist, album);
                exit = false;
                break;
            case 5: {
                std::string desc = getNextUndoDescription();
                if (undoLastAction()) {
                    cout << "Undid: " << (desc.empty() ? "last action" : desc) << endl;
                } else {
                    cout << "Nothing to undo." << endl;
                }
                system("pause");
                exit = false;
                break;
            }
            case 6: {
                std::string desc = getNextRedoDescription();
                if (redoLastAction()) {
                    cout << "Redid: " << (desc.empty() ? "last action" : desc) << endl;
                } else {
                    cout << "Nothing to redo." << endl;
                }
                system("pause");
                exit = false;
                break;
            }
            case 7:
                displayStatistics(artist, album);
                exit = true;
                break;
            default:
                exit = false;
                break;
        }
    }while (!exit);
}

//10
int mainMenu()
{
    return MenuView::mainMenu();
}

void displayStatistics(const artistList& artist, const albumList& album)
{
    system("cls");
    cout << "\n\n\t\t\tSTATISTICS\n\n";
    cout << "Total Artists: " << artist.artList.size() << endl;
    cout << "Total Albums: " << album.albList.size() << endl;
    cout << "\nAlbums per Artist:\n";
    for (const auto& art : artist.artList) {
        int count = 0;
        for (const auto& alb : album.albList) {
            if (alb.artistId == art.artistId) {
                count++;
            }
        }
        cout << art.name << ": " << count << " albums" << endl;
    }
    cout << endl << endl;
    system("pause");
}

void exportArtistsToCSV(const artistList& artist, const std::string& filename) {
    cout << "Exporting artists..." << endl;
    std::ofstream file(filename);
    if (!file) {
        std::cout << "Error opening file for export." << std::endl;
        return;
    }
    file << "ID,Name,Gender,Phone,Email\n";
    std::fstream ArtFile;
    try {
        openFile(ArtFile, artistFilePath);
    } catch(const FileException& e) {
        std::cout << "Error opening artist file." << std::endl;
        return;
    }
    ArtistFile artFile;
    for (const auto& art : artist.artList) {
        ArtFile.seekg(art.pos, ios::beg);
        ArtFile.read((char*)&artFile, sizeof(artFile));
        artFile.phones[14] = '\0';
        artFile.emails[49] = '\0';
        file << art.artistId << "," << art.name << "," << artFile.genders << "," << artFile.phones << "," << artFile.emails << "\n";
    }
    ArtFile.close();
    file.close();
    std::cout << "Artists exported to " << filename << std::endl;
}

void exportAlbumsToCSV(const albumList& album, const std::string& filename) {
    cout << "Exporting albums..." << endl;
    std::ofstream file(filename);
    if (!file) {
        std::cout << "Error opening file for export." << std::endl;
        return;
    }
    file << "AlbumID,ArtistID,Title,RecordFormat,DatePublished,Path\n";
    std::fstream AlbFile;
    try {
        openFile(AlbFile, albumFilePath);
    } catch(const FileException& e) {
        std::cout << "Error opening album file." << std::endl;
        return;
    }
    AlbumFile albFile;
    for (const auto& alb : album.albList) {
        AlbFile.seekg(alb.pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        albFile.titles[79] = '\0';
        albFile.recordFormats[11] = '\0';
        albFile.datePublished[10] = '\0';
        albFile.paths[99] = '\0';
        file << alb.albumId << "," << alb.artistId << "," << albFile.titles << "," << albFile.recordFormats << "," << albFile.datePublished << "," << albFile.paths << "\n";
    }
    AlbFile.close();
    file.close();
    std::cout << "Albums exported to " << filename << std::endl;
}

//11
bool artistManager(fstream & ArtFile, fstream & AlbFile, artistList &artist, albumList &album, indexSet & result, indexSet & delArtArray, indexSet & delAlbArray)
{
    bool exit;
    do
    {
        exit=false;
        int choice=MenuView::artistMenu();
        if (choice ==1)
            exit=artistViewer(ArtFile,artist,result);
        if (choice ==2)
            exit=artistEditor(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
        if (choice ==3) {
            exportArtistsToCSV(artist, "artists.csv");
            cout << endl << endl;
            system("pause");
            exit = true;
        }
        if (choice ==4)
            return false;
        if (choice ==5) {
            displayStatistics(artist, album);
            return true;
        }
    }while(exit);
    return true;
}

//12
int artistMenu()
{
    int c;
    do{
        system("COLOR 3E");
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *ArtistMenu*               ";
        cout<<"\n\n                       Enter  1 :  >> Artist Viewer                           ";
        cout<<"\n\n                       Enter  2 :  >> Artist Editor                            ";
        cout<<"\n\n                       Enter  3 :  >> Export Artists to CSV                              ";
        cout<<"\n\n                       Enter  4 :  >> Go To Main Menu                              ";
        cout<<"\n\n                       Enter  5 :  >> EXIT.                              \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>5 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>5 || c<1);
    return c;
}

//13
bool artistViewer(std::fstream& ArtFile, const artistList& artist, indexSet& result)
{
    bool exit=false;
    do
    {
        int choice=MenuView::viewArtistMenu();
        if (choice ==1)
            displayAllArtist(ArtFile,artist);
        if (choice ==2)
            viewArtistBySearch(ArtFile,artist,result);
        if (choice ==3)
            exit=true;
    }while(!exit);
    return true;
}

//14
int viewArtistMenu()
{
    int c;
    system("COLOR 4E");
    do{
    system("cls");
    cout<<"\n\n";
    cout<<"\n                                 *View ArtistMenu*               ";
    cout<<"\n\n                       Enter  1 :  >> Display All Artist    ";
    cout<<"\n\n                       Enter  2 :  >> View Artist By Search  ";
    cout<<"\n\n                       Enter  3 :  >> GO BACK.          \n\n ";
    cout<<"\n choice:    ";
    cin>>c;
    cin.clear();
    cin.ignore(INT_MAX,'\n');
    if (c>3 || c<1){
        cout<<"Wrong Choice!";
        cout<<endl<<endl;
        system ("pause");
        system ("cls");
    }
    }while(c>3 || c<1);
    return c;
}

//15
void displayAllArtist(std::fstream& ArtFile, const artistList& artist)
{
    ArtistView::displayAll(artist);
}

//16
void viewArtistBySearch(std::fstream& ArtFile, const artistList& artist, indexSet& result)
{
    searchArtist(ArtFile,artist,result);
    displaySearchResult(ArtFile,artist,result);
    cout<<endl<<endl;
    system("pause");
}

//17
bool searchArtist(std::fstream& ArtFile, const artistList& artist, indexSet& result)
{
    std::string targetId, targetName;
    int searchby = 0;
    cout << "\t1. Search artist by ID \n\t2. Search artist by Name" << endl;
    do{
        cout << "\t  Choice: ";
        if (!(cin >> searchby)) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid input. Please enter 1 or 2.\n";
            searchby = 0;
            continue;
        }
        cin.ignore(INT_MAX, '\n');
        switch (searchby){
        case 1:
            cout << "\nEnter prefix of Id of Artist: ";
            getline(cin, targetId);
            if (!searchArtistById(artist, result, targetId))
                return false;
            break;
        case 2:
            cout << "\nEnter prefix of name of Artist: ";
            getline(cin, targetName);
            if (!searchArtistByName(artist, result, targetName))
                return false;
            break;
        default:
            cout << "Wrong choice. Enter 1 or 2.\n";
            break;
        }
    }while(searchby != 1 && searchby != 2);

    return true;
}

//18
bool searchArtistById(const artistList& artist, indexSet& result, const std::string& targetId)
{
    result.indexes.clear();
    for(size_t i = 0; i < artist.artList.size(); i++){
        if(artist.artList[i].artistId != "-1" && artist.artList[i].artistId.find(targetId) == 0){
            result.indexes.push_back(i);
        }
    }
    return !result.indexes.empty();
}

//19
bool searchArtistByName(const artistList& artist, indexSet& result, const std::string& targetName)
{
    result.indexes.clear();
    std::string lowerTarget = targetName;
    std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::tolower);
    for(size_t i = 0; i < artist.artList.size(); i++){
        if(artist.artList[i].artistId != "-1"){
            std::string lowerName = artist.artList[i].name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if(lowerName.find(lowerTarget) == 0){
                result.indexes.push_back(i);
            }
        }
    }
    return !result.indexes.empty();
}

//20
void displaySearchResult(std::fstream& ArtFile, const artistList& artist, const indexSet& result)
{
    ArtistView::displaySearchResult(artist, result);
}

//21
bool artistEditor(fstream & ArtFile, fstream & AlbFile, artistList &artist, albumList &album, indexSet & result, indexSet & delArtArray, indexSet & delAlbArray)
{
    bool exit=false, success=false;
    do
    {
        int choice=MenuView::editArtistMenu();
        if (choice ==1){
            success= addArtist(ArtFile,artist);
            if(success)
                cout<<"Artist Added Successfully! "<<endl;
            else
                cout<<"Artist not added. "<<endl;
        cout<<endl<<endl;
        system("pause");
        }
        if (choice ==2)
            editArtist(ArtFile,artist,result);
        if (choice ==3)
            deleteArtist(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
        if (choice ==4)
            exit=true;
    }while(!exit);
    return true;
}

//22
int editArtistMenu()
{
    int c;
    system("COLOR 2E");
    do{
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Edit Artist Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> ADD Artist    ";
        cout<<"\n\n                       Enter  2 :  >> Edit Artist  ";
        cout<<"\n\n                       Enter  3 :  >> Delete Artist  ";
        cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>4 || c<1){
            cout<<"Wrong Choice!"<<endl;
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
            }
    }while(c>4 || c<1);
    return c;
}

//22
bool getAddConfirmation(const std::string& itemType)
{
    char addA;
    system("cls");
    cout << "Do you want to add an " << itemType << "? (Y/N) : ";
    cin >> addA;
    cin.ignore(INT_MAX, '\n');

    if (addA == 'y' || addA == 'Y')
        return true;
    else if (addA == 'n' || addA == 'N')
        return false;
    else {
        cout << "Wrong entry. Try again!" << endl;
        return false;
    }
}

//23
CommandAction createAddArtistCommand(Artist art, std::fstream& ArtFile, artistList& artist)
{
    auto state = std::make_shared<ArtistCommandState>();
    state->artist = art;

    CommandAction action;
    action.description = "Add artist " + art.getName();
    action.redo = [&, state]() -> bool {
        if (state->pos < 0) {
            if (!appendArtistRecord(ArtFile, state->artist, state->pos)) {
                return false;
            }
        } else {
            if (!writeArtistAtPosition(ArtFile, state->pos, state->artist)) {
                return false;
            }
        }

        int idx = findArtistIndexById(artist, state->artist.getArtistId());
        if (idx == -1) {
            artist.artList.push_back({state->artist.getArtistId(), state->artist.getName(), state->pos});
        } else {
            artist.artList[idx].artistId = state->artist.getArtistId();
            artist.artList[idx].name = state->artist.getName();
            artist.artList[idx].pos = state->pos;
        }
        sortArtist(artist);
        Logger::getInstance()->log("Redo add artist: " + state->artist.getName());
        return true;
    };
    action.undo = [&, state]() {
        int idx = findArtistIndexById(artist, state->artist.getArtistId());
        if (idx == -1) {
            return;
        }
        if (ensureArtistStream(ArtFile)) {
            ArtistFile blank = {"-1", "", 'N', "", ""};
            ArtFile.clear();
            ArtFile.seekp(state->pos, ios::beg);
            ArtFile.write(reinterpret_cast<const char*>(&blank), sizeof(ArtistFile));
            ArtFile.flush();
        }
        artist.artList.erase(artist.artList.begin() + idx);
        Logger::getInstance()->log("Undo add artist: " + state->artist.getName());
    };

    return action;
}

//24
bool addArtist(std::fstream& ArtFile, artistList& artist)
{
    if (!getAddConfirmation("artist"))
        return false;

    Artist art = getArtistInfo();
    std::string id = intToString(++lastArtistID, "art");
    art.setArtistId(id);

    CommandAction action = createAddArtistCommand(art, ArtFile, artist);

    if (!executeCommand(std::move(action))) {
        --lastArtistID;
        Logger::getInstance()->log("Failed to add artist via command");
        return false;
    }
    return true;
}

//24
Artist getArtistInfo()
{
    Artist art;
    art.setName(getArtistName());
    art.setGender(getArtistGender());
    art.setPhone(getArtistPhone());
    art.setEmail(getArtistEmail());
    return art;
}

//25
std::string getArtistName()
{
    std::string name;
    while (true)
    {
        cout << "Enter Artist name: ";
        if (!getline(cin, name)) {
            cin.clear();
            continue;
        }
        try {
            validateName(name);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }
    name = formatName(name);
    return name;
}

//26
char getArtistGender()
{
    char gender;
    do
    {
        cout<<"Enter Artist Gender (M/F): ";
        cin>>gender;
        cin.ignore(INT_MAX, '\n'); // Consume the newline
        if (gender>='a' && gender<='z'){
                gender-=32;}
        try {
            validateGender(gender);
            return gender;
        } catch(const ValidationException& e) {
            cout<<e.what()<<endl;
        }
    }while(true);
}

//27
std::string getArtistPhone()
{
    std::string phone;
    while (true)
    {
        cout << "Enter Artist Phone Number: ";
        if (!getline(cin, phone)) {
            cin.clear();
            continue;
        }
        try {
            validatePhone(phone);
            return phone;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }
}

//28
std::string getArtistEmail()
{
    std::string email;
    do
    {
        cout << "<sample@email.com> or <sample@email> \nEnter Artist email: ";
        getline(cin, email);
        try {
            validateEmail(email);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    email = formatEmail(email);
    return email;
}

//29.	validateName
void validateName(const std::string& name)
{
    if (name.empty()){
        throw ValidationException("Artist name cannot be empty!");
    }else if (name[0] == ' '){
        throw ValidationException("Artist name cannot start with a space!");
    }else{
        for(char c : name){
            if(!(c == ' ' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))){
                throw ValidationException("Artist name contains invalid characters!");
            }
        }
    }
}

//30
std::string formatName(std::string name)
{
    for (size_t i = 0; i < name.length(); i++)
    {
        if(i == 0 || name[i-1] == ' '){
            if (name[i] >= 'a' && name[i] <= 'z'){
                name[i] -= 32;}
        }else{
            if(name[i] >= 'A' && name[i] <= 'Z'){
                name[i] += 32;}
        }
    }
    return name;
}

//31
void validateGender(char gender)
{
    if (!(gender =='M' || gender =='F'))
        throw ValidationException("Artist gender should be male(M) or female(F)!");
}

//32
void validatePhone(const std::string& phone)
{
    if (phone.empty())
        throw ValidationException("Phone number cannot be empty!");

    if (phone.length() < 10 || phone.length() > 15)
        throw ValidationException("Phone number must contain between 10 and 15 digits!");

    for(char c : phone){
        if(c < '0' || c > '9'){
            throw ValidationException("Phone number must contain only digits!");
        }
    }
}

//33
void validateEmail(const std::string& email)
{
    int domain = 0;
    if (email.empty()){
        throw ValidationException("Email cannot be empty!");
    }else if (email[0] == ' ' || email[0] == '@'){
        throw ValidationException("Email cannot start with space or @!");
    }else{
        for(char c : email){
            if (c == ' ')
                throw ValidationException("Email cannot contain spaces!");
            if (c == '@')
                domain++;
        }
        if(domain != 1)
            throw ValidationException("Email must contain exactly one @!");

        size_t atPos = email.find('@');
        std::string localPart = email.substr(0, atPos);
        std::string domainPart = email.substr(atPos + 1);

        if (localPart.length() < 2)
            throw ValidationException("Email local part must have at least 2 characters!");
        if (domainPart.length() < 3)
            throw ValidationException("Email domain must contain a valid host and extension!");

        auto isValidChar = [](char c){
            return (c >= '0' && c <= '9') ||
                   (c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z') ||
                   c == '.' || c == '-' || c == '_';
        };

        if (!std::all_of(localPart.begin(), localPart.end(), isValidChar))
            throw ValidationException("Email local part contains invalid characters!");
        if (!std::all_of(domainPart.begin(), domainPart.end(), isValidChar))
            throw ValidationException("Email domain contains invalid characters!");

        size_t lastDot = domainPart.rfind('.');
        if (lastDot == std::string::npos || lastDot == 0 || lastDot == domainPart.length() - 1)
            throw ValidationException("Email domain must contain a '.' followed by a valid extension!");

        std::string hostPart = domainPart.substr(0, lastDot);
        std::string tldPart = domainPart.substr(lastDot + 1);

        if (hostPart.empty())
            throw ValidationException("Email domain must include a host name before the '.'!");
        if (tldPart.length() < 2 || !std::all_of(tldPart.begin(), tldPart.end(), [](char c){ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }))
            throw ValidationException("Email domain extension must be at least two letters!");
    }
}

//34
std::string formatEmail(std::string email)
{
    size_t pos = email.find(".com");
    if (pos != std::string::npos){
        // already has .com
    } else {
        email += ".com";
    }
    for(char& c : email){
        if (c >= 'A' && c <= 'Z')
            c += 32;
    }
    return email;
}

//35
void editArtist(std::fstream& ArtFile, artistList& artist, indexSet& result)
{
    system("cls");
    cout << setw(30) << "Edit Artist " << endl;
    do{
        searchArtist(ArtFile, artist, result);
        if (result.indexes.empty()){
            printError(4);
            system("pause");
            return;
        }
    }while(result.indexes.empty());
    size_t idx = selectArtist(ArtFile, artist, result, "edit");
    editArtistInfo(ArtFile, artist, idx);
    sortArtist(artist);
}

//36
int selectArtist(std::fstream& ArtFile, const artistList& artist, indexSet& result, const std::string& forWhat)
{
    if (result.indexes.empty()) {
        return -1;
    }

    cout << result.indexes.size() << " results found.\n";
    system("pause");
    for (size_t i = 0; i < result.indexes.size(); ++i) {
        cout << '\t' << i + 1 << ". " << artist.artList[result.indexes[i]].name << endl;
    }

    int selection = 0;
    cout << "\n\t Select Artist to " << forWhat << ':';
    do {
        if (!(cin >> selection)) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid input. Enter a number between 1 and " << result.indexes.size() << "." << endl;
            continue;
        }
        if (selection < 1 || selection > static_cast<int>(result.indexes.size())) {
            cout << "Wrong choice. Try again." << endl;
        }
    } while (selection < 1 || selection > static_cast<int>(result.indexes.size()));
    cin.ignore(INT_MAX, '\n');
    return result.indexes[selection - 1];
}

//37
bool editArtistInfo(std::fstream& ArtFile, artistList& artist, int idx)
{
    displayOneArtist(ArtFile, artist, idx);
    long pos = artist.artList[idx].pos;
    Artist original;
    if (!readArtistAtPosition(ArtFile, pos, original)) {
        cout << "Failed to read artist data." << endl;
        system("pause");
        Logger::getInstance()->log("Failed to read artist for editing at position: " + std::to_string(pos));
        return false;
    }

    Artist updated = getArtistInfo();
    updated.setArtistId(original.getArtistId());

    auto state = std::make_shared<ArtistEditState>();
    state->original = original;
    state->updated = updated;
    state->pos = pos;

    CommandAction action;
    action.description = "Edit artist " + original.getName();
    action.redo = [&, state]() -> bool {
        if (!writeArtistAtPosition(ArtFile, state->pos, state->updated)) {
            Logger::getInstance()->log("Failed to apply artist edit during redo");
            return false;
        }
        int targetIdx = findArtistIndexById(artist, state->updated.getArtistId());
        if (targetIdx != -1) {
            artist.artList[targetIdx].name = state->updated.getName();
            artist.artList[targetIdx].pos = state->pos;
        }
        sortArtist(artist);
        state->applied = true;
        Logger::getInstance()->log("Redo edit artist: " + state->updated.getName());
        return true;
    };
    action.undo = [&, state]() {
        if (!writeArtistAtPosition(ArtFile, state->pos, state->original)) {
            Logger::getInstance()->log("Failed to revert artist edit during undo");
            return;
        }
        int targetIdx = findArtistIndexById(artist, state->original.getArtistId());
        if (targetIdx != -1) {
            artist.artList[targetIdx].name = state->original.getName();
            artist.artList[targetIdx].pos = state->pos;
        }
        sortArtist(artist);
        state->applied = false;
        Logger::getInstance()->log("Undo edit artist: " + state->original.getName());
    };

    if (!executeCommand(std::move(action))) {
        cout << "Failed to edit artist." << endl;
        system("pause");
        return false;
    }

    cout << "\n\tEdited \n\n";
    system("pause");
    return true;
}

//38
void displayOneArtist(std::fstream& ArtFile, const artistList& artist, int idx)
{
    ArtistView::displayOne(artist, idx);
}

//39
void deleteArtist(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray)
{
    system("cls");
    cout << setw(30) << "Delete Artist " << endl;
    result.indexes.clear();
    if (!searchArtist(ArtFile, artist, result)) {
        printError(4);
        system("pause");
        return;
    }

    int selectedIdx = selectArtist(ArtFile, artist, result, "delete");
    if (selectedIdx < 0) {
        return;
    }
    displayOneArtist(ArtFile, artist, selectedIdx);
    removeArtist(ArtFile, AlbFile, artist, album, delArtArray, delAlbArray, selectedIdx);
}

//39
ArtistRemovalState prepareArtistRemovalState(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, const albumList& album, int idx)
{
    Artist original;
    if (!readArtistAtPosition(ArtFile, artist.artList[idx].pos, original)) {
        cout << "Failed to load artist details." << endl;
        system("pause");
        Logger::getInstance()->log("Failed to read artist during removal");
        throw std::runtime_error("Failed to read artist");
    }

    if (!ensureAlbumStream(AlbFile)) {
        Logger::getInstance()->log("Failed to ready album file for artist removal");
        throw std::runtime_error("Failed to ready album file");
    }

    ArtistRemovalState baseState;
    baseState.artist = original;
    baseState.pos = artist.artList[idx].pos;

    for (size_t i = 0; i < album.albList.size(); ++i) {
        if (album.albList[i].artistId == original.getArtistId()) {
            Album snapshotAlbum;
            if (readAlbumAtPosition(AlbFile, album.albList[i].pos, snapshotAlbum)) {
                AlbumSnapshot snap;
                snap.data = snapshotAlbum;
                snap.pos = album.albList[i].pos;
                baseState.associatedAlbums.push_back(snap);
            }
        }
    }

    return baseState;
}

//40
bool getRemovalConfirmation()
{
    char remv;
    do {
        cout << "Are you sure you want to remove the selected artist? (Y/N) : ";
        cin >> remv;
        if (remv == 'y' || remv == 'Y') {
            return true;
        } else if (remv == 'n' || remv == 'N') {
            cout << "Artist not removed. \n" << endl;
            system("pause");
            Logger::getInstance()->log("Artist removal cancelled by user");
            return false;
        } else {
            cout << "Wrong entry. Try again!" << endl;
        }
    } while (remv != 'y' && remv != 'Y' && remv != 'n' && remv != 'N');
    return false;
}

//41
CommandAction createRemoveArtistCommand(ArtistRemovalState state, std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtArray, indexSet& delAlbArray, int idx)
{
    auto statePtr = std::make_shared<ArtistRemovalState>(state);

    CommandAction action;
    action.description = "Delete artist " + statePtr->artist.getName();
    action.redo = [&, statePtr, idx]() -> bool {
        if (!ensureArtistStream(ArtFile) || !ensureAlbumStream(AlbFile)) {
            return false;
        }

        ArtistFile blankArtist = {"-1", "", 'N', "", ""};
        AlbumFile blankAlbum = {"-1", "-1", "", "", "", ""};

        for (auto& snapshot : statePtr->associatedAlbums) {
            int albumIdx = findAlbumIndexById(album, snapshot.data.getAlbumId());
            if (albumIdx == -1) {
                continue;
            }
            AlbFile.clear();
            AlbFile.seekp(snapshot.pos, ios::beg);
            AlbFile.write(reinterpret_cast<const char*>(&blankAlbum), sizeof(AlbumFile));
            AlbFile.flush();
            album.albList[albumIdx].albumId = "-1";
            album.albList[albumIdx].artistId = "-1";
            album.albList[albumIdx].title = "";
            album.albList[albumIdx].pos = snapshot.pos;
            if (std::find(delAlbArray.indexes.begin(), delAlbArray.indexes.end(), albumIdx) == delAlbArray.indexes.end()) {
                delAlbArray.indexes.push_back(albumIdx);
            }
        }

        int artistIdx = findArtistIndexById(artist, statePtr->artist.getArtistId());
        statePtr->artistIndex = artistIdx;
        if (artistIdx != -1) {
            ArtFile.clear();
            ArtFile.seekp(statePtr->pos, ios::beg);
            ArtFile.write(reinterpret_cast<const char*>(&blankArtist), sizeof(ArtistFile));
            ArtFile.flush();
            artist.artList[artistIdx].artistId = "-1";
            artist.artList[artistIdx].name = "";
            artist.artList[artistIdx].pos = statePtr->pos;
            if (std::find(delArtArray.indexes.begin(), delArtArray.indexes.end(), artistIdx) == delArtArray.indexes.end()) {
                delArtArray.indexes.push_back(artistIdx);
            }
        }

        Logger::getInstance()->log("Redo artist removal: " + statePtr->artist.getName());
        return true;
    };

    action.undo = [&, statePtr]() {
        if (!writeArtistAtPosition(ArtFile, statePtr->pos, statePtr->artist)) {
            Logger::getInstance()->log("Failed to restore artist during undo");
            return;
        }
        int artistIdx = findArtistIndexById(artist, statePtr->artist.getArtistId());
        if (artistIdx != -1) {
            artist.artList[artistIdx].artistId = statePtr->artist.getArtistId();
            artist.artList[artistIdx].name = statePtr->artist.getName();
            artist.artList[artistIdx].pos = statePtr->pos;
            auto it = std::find(delArtArray.indexes.begin(), delArtArray.indexes.end(), artistIdx);
            if (it != delArtArray.indexes.end()) {
                delArtArray.indexes.erase(it);
            }
        }

        for (auto& snapshot : statePtr->associatedAlbums) {
            if (!writeAlbumAtPosition(AlbFile, snapshot.pos, snapshot.data)) {
                continue;
            }
            int albumIdx = findAlbumIndexById(album, snapshot.data.getAlbumId());
            if (albumIdx != -1) {
                album.albList[albumIdx].albumId = snapshot.data.getAlbumId();
                album.albList[albumIdx].artistId = snapshot.data.getArtistId();
                album.albList[albumIdx].title = snapshot.data.getTitle();
                album.albList[albumIdx].pos = snapshot.pos;
                auto itAlb = std::find(delAlbArray.indexes.begin(), delAlbArray.indexes.end(), albumIdx);
                if (itAlb != delAlbArray.indexes.end()) {
                    delAlbArray.indexes.erase(itAlb);
                }
            }
        }

        Logger::getInstance()->log("Undo artist removal: " + statePtr->artist.getName());
    };

    return action;
}

//42
void removeArtist(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtArray, indexSet& delAlbArray, int idx)
{
    Logger::getInstance()->log("Removing artist: " + artist.artList[idx].name + " with ID: " + artist.artList[idx].artistId);

    try {
        ArtistRemovalState state = prepareArtistRemovalState(ArtFile, AlbFile, artist, album, idx);

        if (!getRemovalConfirmation()) {
            return;
        }

        CommandAction action = createRemoveArtistCommand(state, ArtFile, AlbFile, artist, album, delArtArray, delAlbArray, idx);

        if (!executeCommand(std::move(action))) {
            cout << "Failed to remove artist." << endl;
            system("pause");
            return;
        }

        cout << "\n\t Artist removed successfully! \n" << endl;
        system("pause");
    } catch (const std::runtime_error& e) {
        // Error already logged in prepareArtistRemovalState
        return;
    }
}

//43
void removeArtistAllAlbums(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& delAlbArray, int i)
{
    int pos;
    AlbumFile BLANK_ALBUM_FILE = {"-1", "-1", "", "", "", ""};
    if (!AlbFile.is_open()) {
        try {
            openFile(AlbFile, albumFilePath);
        } catch (const FileException& e) {
            cout << e.what() << endl;
            system("pause");
            Logger::getInstance()->log("Failed to open album file for bulk artist removal");
            return;
        }
    }
    AlbFile.clear();
    AlbFile.seekp(album.albList[i].pos, ios::beg);
    pos = AlbFile.tellp();
    AlbFile.write((char*)&BLANK_ALBUM_FILE, sizeof(AlbumFile));
    AlbFile.flush();
    album.albList[i].albumId = BLANK_ALBUM_FILE.albumIds;
    album.albList[i].artistId = BLANK_ALBUM_FILE.artistIdRefs;
    album.albList[i].title = BLANK_ALBUM_FILE.titles;
    album.albList[i].pos = pos;
    delAlbArray.indexes.push_back(i);
}

//42
bool albumManager(fstream & ArtFile, fstream & AlbFile, artistList &artist, albumList &album, indexSet & result, indexSet & delArtArray, indexSet & delAlbArray)
{
    bool exit;
    do
    {
        exit=false;
        int choice=MenuView::albumMenu();
        if (choice ==1)
            exit=albumViewer(AlbFile,album,result);
        if (choice ==2)
            exit=albumEditor(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
        if (choice ==3) {
            exportAlbumsToCSV(album, "albums.csv");
            cout << endl << endl;
            system("pause");
            exit = true;
        }
        if (choice ==4)
            return false;
        if (choice ==5) {
            displayStatistics(artist, album);
            return true;
        }
    }while(exit);
    return true;
}

//43
int albumMenu()
{
    int c;
    do{
        system("COLOR 1B");
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Album Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> Album Viewer                           ";
        cout<<"\n\n                       Enter  2 :  >> Album Editor                            ";
        cout<<"\n\n                       Enter  3 :  >> Export Albums to CSV                              ";
        cout<<"\n\n                       Enter  4 :  >> Go To Main Menu                              ";
        cout<<"\n\n                       Enter  5 :  >> EXIT.                              \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>5 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>5 || c<1);
    return c;
}

//44
bool albumViewer(std::fstream& AlbFile, const albumList& album, indexSet& result)
{
    bool exit=false;
    do
    {
        int choice=MenuView::viewAlbumMenu();
        if (choice ==1)
            displayAllAlbums(AlbFile,album);
        if ( choice == 2 ){
            system("cls");
            std::string target;
            cout << "\nEnter prefix of Id of Artist: ";
            cin >> target;
            if(!searchAlbumByArtistId(AlbFile, album, result, target)){
                printError(4);
                system("pause");
            }
            else{
                displayAlbumSearchResult(AlbFile, album, result);
                cout << endl << endl;
                system("pause");
            }
        }
        if (choice ==3)
            advancedSearchAlbums(AlbFile, album, result);
        if (choice ==4)
            exit=true;
    }while(!exit);
    return true;
}

//45
int viewAlbumMenu()
{
    int c;
    system("COLOR 2E");
    do{
    system("cls");
    cout<<"\n\n";
    cout<<"\n                                 *View Album Menu*               ";
    cout<<"\n\n                       Enter  1 :  >> Display All Albums    ";
    cout<<"\n\n                       Enter  2 :  >> View Artist Albums By Search   ";
    cout<<"\n\n                       Enter  3 :  >> Advanced Search   ";
    cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
    cout<<"\n choice:    ";
    cin>>c;
    cin.clear();
    cin.ignore(INT_MAX,'\n');
    if (c>4 || c<1){
        cout<<"Wrong Choice!";
        cout<<endl<<endl;
        system ("pause");
        system ("cls");
    }
    }while(c>4 || c<1);
    return c;
}

//46
void displayAllAlbums(std::fstream& AlbFile, const albumList& album)
{
    AlbumView::displayAll(AlbFile, album);
}

//47
bool searchAlbumByArtistId(std::fstream& AlbFile, const albumList& album, indexSet& result, const std::string& targetId)
{
    result.indexes.clear();
    for(size_t i = 0; i < album.albList.size(); i++){
        if(album.albList[i].artistId != "-1"){
            if(album.albList[i].artistId.find(targetId) == 0){
               result.indexes.push_back(i);
            }
        }
    }
    return !result.indexes.empty();
}

//48
void displayAlbumSearchResult(std::fstream& AlbFile, const albumList& album, const indexSet& result)
{
    AlbumView::displaySearchResult(AlbFile, album, result);
}

//49
bool albumEditor(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray)
{
    bool exit=false, success=false;
    do
    {
        int choice=MenuView::editAlbumMenu();
        if (choice ==1){
            success=addAlbum(ArtFile,AlbFile,artist,album,result);
            if(success)
                cout<<"Artist Added Successfully! "<<endl;
            else
                cout<<"Artist not added. "<<endl;
        cout<<endl<<endl;
        system("pause");
        }
        if (choice ==2)
            editAlbum(ArtFile,AlbFile,artist,album,result);
        if (choice ==3)
            deleteAlbum(ArtFile,AlbFile,artist,album,result,delAlbArray);
        if (choice ==4)
            exit=true;
    }while(!exit);
    return true;
}

//50
int editAlbumMenu()
{
    int c;
    system("COLOR 2E");
    do{
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Edit Album Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> ADD Album    ";
        cout<<"\n\n                       Enter  2 :  >> Edit Album  ";
        cout<<"\n\n                       Enter  3 :  >> Delete Album  ";
        cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>4 || c<1){
            cout<<"Wrong Choice!"<<endl;
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
            }
    }while(c>4 || c<1);
    return c;
}

//50
int selectArtistForAlbum(std::fstream& ArtFile, const artistList& artist, indexSet& result)
{
    while (result.indexes.empty()) {
        searchArtist(ArtFile, artist, result);
        if (result.indexes.empty()) {
            printError(4);
            system("pause");
        }
    }
    return selectArtist(ArtFile, artist, result, "add an album");
}

//51
CommandAction createAddAlbumCommand(Album album, std::fstream& AlbFile, albumList& albumList)
{
    auto state = std::make_shared<AlbumCommandState>();
    state->album = album;

    CommandAction action;
    action.description = "Add album " + album.getTitle();
    action.redo = [&, state]() -> bool {
        if (state->pos < 0) {
            if (!appendAlbumRecord(AlbFile, state->album, state->pos)) {
                return false;
            }
        } else {
            if (!writeAlbumAtPosition(AlbFile, state->pos, state->album)) {
                return false;
            }
        }

        int idx = findAlbumIndexById(albumList, state->album.getAlbumId());
        if (idx == -1) {
            albumList.albList.push_back(albumIndex{state->album.getAlbumId(), state->album.getArtistId(), state->album.getTitle(), state->pos});
        } else {
            albumList.albList[idx].albumId = state->album.getAlbumId();
            albumList.albList[idx].artistId = state->album.getArtistId();
            albumList.albList[idx].title = state->album.getTitle();
            albumList.albList[idx].pos = state->pos;
        }
        sortAlbum(albumList);
        Logger::getInstance()->log("Redo add album: " + state->album.getTitle());
        return true;
    };
    action.undo = [&, state]() {
        int idx = findAlbumIndexById(albumList, state->album.getAlbumId());
        if (idx == -1) {
            return;
        }
        if (ensureAlbumStream(AlbFile)) {
            AlbumFile blank = {"-1", "-1", "", "", "", ""};
            AlbFile.clear();
            AlbFile.seekp(state->pos, ios::beg);
            AlbFile.write(reinterpret_cast<const char*>(&blank), sizeof(AlbumFile));
            AlbFile.flush();
        }
        albumList.albList.erase(albumList.albList.begin() + idx);
        Logger::getInstance()->log("Undo add album: " + state->album.getTitle());
    };

    return action;
}

//52
bool addAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result)
{
    if (!getAddConfirmation("album"))
        return false;

    int select = selectArtistForAlbum(ArtFile, artist, result);
    AlbumFile albFile = getAlbumInfo();
    std::string idStr = intToString(++lastAlbumID, "alb");
    strncpy(albFile.albumIds, idStr.c_str(), 7);
    albFile.albumIds[7] = '\0';
    strncpy(albFile.artistIdRefs, artist.artList[select].artistId.c_str(), 7);
    albFile.artistIdRefs[7] = '\0';

    Album newAlbum;
    newAlbum.setAlbumId(std::string(albFile.albumIds));
    newAlbum.setArtistId(std::string(albFile.artistIdRefs));
    newAlbum.setTitle(std::string(albFile.titles));
    newAlbum.setRecordFormat(std::string(albFile.recordFormats));
    newAlbum.setDatePublished(std::string(albFile.datePublished));
    newAlbum.setPath(std::string(albFile.paths));

    CommandAction action = createAddAlbumCommand(newAlbum, AlbFile, album);

    if (!executeCommand(std::move(action))) {
        --lastAlbumID;
        Logger::getInstance()->log("Failed to add album via command");
        return false;
    }

    cout << endl;
    cout << " Album ID: " << newAlbum.getAlbumId() << endl;
    cout << endl << endl;
    result.indexes.clear();
    return true;
}

//52
AlbumFile getAlbumInfo()
{
    AlbumFile albFile;
    std::string title = getAlbumTitle();
    std::string recordFormat = getAlbumRecordFormat();
    std::string date = getAlbumDate();
    std::string path = getAlbumPath();
    strncpy(albFile.titles, title.c_str(), 79);
    albFile.titles[79] = '\0';
    strcpy(albFile.recordFormats, recordFormat.c_str());
    strncpy(albFile.datePublished, date.c_str(), 10);
    albFile.datePublished[10] = '\0';
    strcpy(albFile.paths, path.c_str());
    return albFile;
}

//53
std::string getAlbumTitle()
{
    std::string title;
    do
    {
        cout << "Enter album title: ";
        getline(cin, title);
        try {
            validateAlbumTitle(title);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    title = formatAlbumTitle(title);
    return title;
}

//54
std::string getAlbumRecordFormat()
{
    std::string albumFormat;
    do{
        cout << "Enter the record format of the album: ";
        getline(cin, albumFormat);
        try {
            validateAlbumFormat(albumFormat);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    albumFormat = formatAlbumFormat(albumFormat);
    return albumFormat;
}

//55
std::string getAlbumDate()
{
    unsigned int day, month, year;
    do{
        cout << "Enter the date published (DD/MM/YYYY): ";
        if (!(cin >> day) || cin.get() != '/' || !(cin >> month) || cin.get() != '/' || !(cin >> year)) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid date format. Please enter in DD/MM/YYYY format." << endl;
            continue;
        }
        cin.ignore(INT_MAX, '\n');
        try {
            validateAlbumDate(day, month, year);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    return formatAlbumDate(day, month, year);
}

//56
std::string getAlbumPath()
{
    std::string albumPath;
    do{
        cout << "Enter album path: ";
        getline(cin, albumPath);
        try {
            validateAlbumPath(albumPath);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    albumPath = formatAlbumPath(albumPath);
    return albumPath;
}

//57
void validateAlbumTitle(const std::string& albumTitle)
{
    if (albumTitle.empty()){
        throw ValidationException("Album title cannot be empty!");
    }else if (albumTitle[0] == ' '){
        throw ValidationException("Album title cannot start with a space!");
    }else{
        for(char c : albumTitle){
            if(!(c == ' ' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))){
                throw ValidationException("Album title contains invalid characters!");
            }
        }
    }
}

std::string formatAlbumTitle(std::string albumTitle)
{
    for (size_t i = 0; i < albumTitle.length(); i++)
    {
        if(i == 0 || albumTitle[i-1] == ' '){
            if (albumTitle[i] >= 'a' && albumTitle[i] <= 'z'){
                albumTitle[i] -= 32;
            }
        }else{
            if(albumTitle[i] >= 'A' && albumTitle[i] <= 'Z'){
                albumTitle[i] += 32;
            }
        }
    }
    return albumTitle;
}

//59
void validateAlbumFormat(const std::string& albumFormat)
{
    std::string lowerFormat = albumFormat;
    for(char& c : lowerFormat) c = tolower(c);
    std::vector<std::string> validFormats = {"m4a", "flac", "mp3", "mp4", "wav", "wma", "aac", "dsd", "alac", "aiff"};
    for(const auto& fmt : validFormats){
        if(lowerFormat == fmt) return;
    }
    throw ValidationException("Invalid album record format!");
}

//60
std::string formatAlbumFormat(std::string albumFormat)
{
    for (char& c : albumFormat){
        if (c >= 'A' && c <= 'Z')
            c += 32;
    }
    return albumFormat;
}

//61
void validateAlbumDate(unsigned int day, unsigned int month, unsigned int year)
{
    if (month>12 ||day<1 || month<1 || year<0 )
        throw ValidationException("Invalid date: month/day/year out of range!");
    if(month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12){
        if (day>31)
            throw ValidationException("Invalid date: day exceeds 31 for the month!");
    }else if (day>30)
        throw ValidationException("Invalid date: day exceeds 30 for the month!");
    if(month==2)
    {
        if (year%4==0){
            if((year%100==0) && (year%400!=0)){  //Leap Year
                if(day>28)
                    throw ValidationException("Invalid date: February has only 28 days in this year!");
            }else if (day>29)
                throw ValidationException("Invalid date: February has only 29 days in leap year!");
        }else if (day>28)
                throw ValidationException("Invalid date: February has only 28 days!");
    }
}

//62
std::string formatAlbumDate(unsigned int day, unsigned int month, unsigned int year)
{
    char tempD[3], tempM[3], tempY[5];
    sprintf(tempD, "%02d", day);
    sprintf(tempM, "%02d", month);
    sprintf(tempY, "%04d", year);
    std::string date = std::string(tempD) + "/" + std::string(tempM) + "/" + std::string(tempY);
    return date;
}

//63
void validateAlbumPath(const std::string& albumPath)
{
    int slash = 0;
    if (albumPath.empty()){
        throw ValidationException("Album path cannot be empty!");
    }else if (albumPath[0] == ' '){
        throw ValidationException("Album path cannot start with a space!");
    }else{
        for(char c : albumPath){
            if (c == '\\')
                slash++;
        }
        if(slash < 1)
            throw ValidationException("Album path must contain at least one backslash!");
    }
}

//64
std::string formatAlbumPath(std::string albumPath)
{
    for (size_t i = 0; i < albumPath.length(); i++)
    {
        if(i == 0 || albumPath[i-1] == '\\'){
            if (albumPath[i] >= 'a' && albumPath[i] <= 'z'){
                albumPath[i] -= 32;
            }
        }else{
            if(albumPath[i] >= 'A' && albumPath[i] <= 'Z'){
                albumPath[i] += 32;
            }
        }
    }
    return albumPath;
}

//65
void editAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result)
{
    system("cls");
    cout << setw(30) << "Edit Album " << endl;
    int select;
    bool finish = false;
   while(result.indexes.empty()){
        searchArtist(ArtFile, artist, result);
        select = selectArtist(ArtFile, artist, result, "edit");
   }
    select = selectAlbum(AlbFile, artist, album, result, select, "edit");
    if(select == -1 )
        return;
    while (finish == false && !result.indexes.empty())
        finish = editAlbumInfo(AlbFile, album, select);
    sortAlbum(album);
}

//66
int selectAlbum(std::fstream& AlbFile, const artistList& artist, const albumList& album, indexSet& result, int idx, const std::string& forWhat)
{
    int s;
    result.indexes.clear();
    for(size_t i = 0; i < album.albList.size(); i++){
        if(album.albList[i].artistId != "-1"){
            if(artist.artList[idx].artistId == album.albList[i].artistId){
               result.indexes.push_back(i);
            }
        }
    }
    if(result.indexes.size() > 0)
        cout << endl << "   " << result.indexes.size() << " albums have been found." << endl << endl;
    else {
        cout << endl << "   The artist has no album. Please add an album first." << endl << endl;
        system("pause");
        return -1;
    }
    system("pause");
    if (result.indexes.size() == 1){
        displayOneAlbum(AlbFile, album, result.indexes[0]);
        return result.indexes[0];
    }
    else if (result.indexes.size() > 1){
        cout << "   Choose an album to " << forWhat << endl;
        for(size_t i = 0; i < result.indexes.size(); i++){
            cout << "       " << i+1 << ". " << album.albList[result.indexes[i]].title << endl;
        }
        cout << endl;
        do{
            cin >> s;
            if( s < 1 || s > (int)result.indexes.size() ){
                cout << "\tError~Choice must be between 1 and " << result.indexes.size() << ".Re-enter." << endl;
                system("pause");
            }
        }while( s < 1 || s > (int)result.indexes.size() );
        return result.indexes[s-1];
    }
    return -1;
}

//67
bool editAlbumInfo(std::fstream& AlbFile, albumList& album, int idx)
{
    Album original;
    if (!readAlbumAtPosition(AlbFile, album.albList[idx].pos, original)) {
        cout << "Failed to load album details." << endl;
        system("pause");
        Logger::getInstance()->log("Failed to read album during edit");
        return false;
    }

    AlbumFile albFile = getAlbumInfo();
    strncpy(albFile.albumIds, album.albList[idx].albumId.c_str(), 7);
    albFile.albumIds[7] = '\0';
    strncpy(albFile.artistIdRefs, album.albList[idx].artistId.c_str(), 7);
    albFile.artistIdRefs[7] = '\0';

    Album updated;
    updated.setAlbumId(std::string(albFile.albumIds));
    updated.setArtistId(std::string(albFile.artistIdRefs));
    updated.setTitle(std::string(albFile.titles));
    updated.setRecordFormat(std::string(albFile.recordFormats));
    updated.setDatePublished(std::string(albFile.datePublished));
    updated.setPath(std::string(albFile.paths));

    auto state = std::make_shared<AlbumEditState>();
    state->original = original;
    state->updated = updated;
    state->pos = album.albList[idx].pos;

    CommandAction action;
    action.description = "Edit album " + original.getTitle();
    action.redo = [&, state]() -> bool {
        if (!writeAlbumAtPosition(AlbFile, state->pos, state->updated)) {
            Logger::getInstance()->log("Failed to apply album edit during redo");
            return false;
        }
        int albumIdx = findAlbumIndexById(album, state->updated.getAlbumId());
        if (albumIdx != -1) {
            album.albList[albumIdx].albumId = state->updated.getAlbumId();
            album.albList[albumIdx].artistId = state->updated.getArtistId();
            album.albList[albumIdx].title = state->updated.getTitle();
            album.albList[albumIdx].pos = state->pos;
        }
        sortAlbum(album);
        state->applied = true;
        Logger::getInstance()->log("Redo edit album: " + state->updated.getTitle());
        return true;
    };
    action.undo = [&, state]() {
        if (!writeAlbumAtPosition(AlbFile, state->pos, state->original)) {
            Logger::getInstance()->log("Failed to restore album during undo");
            return;
        }
        int albumIdx = findAlbumIndexById(album, state->original.getAlbumId());
        if (albumIdx != -1) {
            album.albList[albumIdx].albumId = state->original.getAlbumId();
            album.albList[albumIdx].artistId = state->original.getArtistId();
            album.albList[albumIdx].title = state->original.getTitle();
            album.albList[albumIdx].pos = state->pos;
        }
        sortAlbum(album);
        state->applied = false;
        Logger::getInstance()->log("Undo edit album: " + state->original.getTitle());
    };

    if (!executeCommand(std::move(action))) {
        cout << "Failed to edit album." << endl;
        system("pause");
        return false;
    }

    cout << "\n\tEdited\n\n";
    system("pause");
    return true;
}

//68
void displayOneAlbum(std::fstream& AlbFile, const albumList& album, int idx)
{
    AlbumView::displayOne(AlbFile, album, idx);
}

/**
 * @brief Helper function to select an artist for album deletion
 * @param ArtFile Reference to the artist file stream
 * @param artist Reference to the artist list
 * @param result Reference to the search result set
 * @return Index of the selected artist
 */
int selectArtistForAlbumDeletion(std::fstream& ArtFile, const artistList& artist, indexSet& result)
{
    searchArtist(ArtFile, artist, result);
    if (result.indexes.empty()) {
        printError(4);
        system("pause");
        return -1;
    }
    return selectArtist(ArtFile, artist, result, "Delete");
}

/**
 * @brief Helper function to get confirmation for deleting all albums of an artist
 * @return User's confirmation choice
 */
char getDeleteAllAlbumsConfirmation()
{
    char answer;
    cout << "Do you want to remove all the albums of this artist?(Y/N): ";
    cin >> answer;
    return answer;
}

/**
 * @brief Helper function to prepare state for deleting all albums of an artist
 * @param ArtFile Reference to the artist file stream
 * @param artist Reference to the artist list
 * @param album Reference to the album list
 * @param result Reference to the search result set
 * @param idx Index of the selected artist
 * @return Shared pointer to vector of album snapshots
 */
std::shared_ptr<std::vector<AlbumSnapshot>> prepareAllAlbumsDeletionState(std::fstream& ArtFile, const artistList& artist, const albumList& album, const indexSet& result, int idx)
{
    std::vector<AlbumSnapshot> snapshots;
    for (const auto& albIdx : result.indexes) {
        if (album.albList[albIdx].artistId != artist.artList[idx].artistId) {
            continue;
        }
        Album snapshotAlbum;
        if (readAlbumAtPosition(ArtFile, album.albList[albIdx].pos, snapshotAlbum)) {
            AlbumSnapshot snap;
            snap.data = snapshotAlbum;
            snap.pos = album.albList[albIdx].pos;
            snapshots.push_back(snap);
        }
    }
    return std::make_shared<std::vector<AlbumSnapshot>>(std::move(snapshots));
}

/**
 * @brief Helper function to prepare state for deleting a single album
 * @param AlbFile Reference to the album file stream
 * @param artist Reference to the artist list
 * @param album Reference to the album list
 * @param result Reference to the search result set
 * @param artistIdx Index of the selected artist
 * @return Pair containing album index and shared pointer to album removal state
 */
std::pair<int, std::shared_ptr<AlbumRemovalState>> prepareSingleAlbumDeletionState(std::fstream& AlbFile, const artistList& artist, const albumList& album, indexSet& result, int artistIdx)
{
    int albumIdx = selectAlbum(AlbFile, artist, album, result, artistIdx, "Delete");
    if (albumIdx == -1) {
        return {-1, nullptr};
    }

    char answer;
    cout << "Are you sure?(Y/N): ";
    cin >> answer;
    if (answer != 'y' && answer != 'Y') {
        cout << "\n\t Failed!\n\n";
        system("pause");
        return {-1, nullptr};
    }

    Album snapshotAlbum;
    if (!readAlbumAtPosition(AlbFile, album.albList[albumIdx].pos, snapshotAlbum)) {
        cout << "Failed to load album." << endl;
        system("pause");
        return {-1, nullptr};
    }

    auto state = std::make_shared<AlbumRemovalState>();
    state->album = snapshotAlbum;
    state->pos = album.albList[albumIdx].pos;
    state->index = albumIdx;

    return {albumIdx, state};
}

/**
 * @brief Helper function to create command action for deleting all albums of an artist
 * @param state Shared pointer to vector of album snapshots
 * @param artist Reference to the artist list
 * @param album Reference to the album list
 * @param delAlbArray Reference to the deleted album array
 * @param idx Index of the selected artist
 * @return CommandAction for the deletion operation
 */
CommandAction createDeleteAllAlbumsCommand(std::shared_ptr<std::vector<AlbumSnapshot>> state, const artistList& artist, albumList& album, indexSet& delAlbArray, int idx)
{
    CommandAction action;
    action.description = "Delete all albums for artist " + artist.artList[idx].name;
    action.redo = [&, state]() -> bool {
        std::fstream AlbFile;
        if (!ensureAlbumStream(AlbFile)) {
            return false;
        }
        AlbumFile blank = {"-1", "-1", "", "", "", ""};
        for (const auto& snapshot : *state) {
            int albumIdx = findAlbumIndexById(album, snapshot.data.getAlbumId());
            if (albumIdx == -1) {
                continue;
            }
            AlbFile.clear();
            AlbFile.seekp(snapshot.pos, ios::beg);
            AlbFile.write(reinterpret_cast<const char*>(&blank), sizeof(AlbumFile));
            AlbFile.flush();
            album.albList[albumIdx].albumId = "-1";
            album.albList[albumIdx].artistId = "-1";
            album.albList[albumIdx].title = "";
            album.albList[albumIdx].pos = snapshot.pos;
            if (std::find(delAlbArray.indexes.begin(), delAlbArray.indexes.end(), albumIdx) == delAlbArray.indexes.end()) {
                delAlbArray.indexes.push_back(albumIdx);
            }
        }
        Logger::getInstance()->log("Redo delete all albums for artist");
        return true;
    };
    action.undo = [&, state]() {
        std::fstream AlbFile;
        for (const auto& snapshot : *state) {
            if (!writeAlbumAtPosition(AlbFile, snapshot.pos, snapshot.data)) {
                continue;
            }
            int albumIdx = findAlbumIndexById(album, snapshot.data.getAlbumId());
            if (albumIdx != -1) {
                album.albList[albumIdx].albumId = snapshot.data.getAlbumId();
                album.albList[albumIdx].artistId = snapshot.data.getArtistId();
                album.albList[albumIdx].title = snapshot.data.getTitle();
                album.albList[albumIdx].pos = snapshot.pos;
                auto itAlb = std::find(delAlbArray.indexes.begin(), delAlbArray.indexes.end(), albumIdx);
                if (itAlb != delAlbArray.indexes.end()) {
                    delAlbArray.indexes.erase(itAlb);
                }
            }
        }
        Logger::getInstance()->log("Undo delete all albums for artist");
    };
    return action;
}

/**
 * @brief Helper function to create command action for deleting a single album
 * @param state Shared pointer to album removal state
 * @param album Reference to the album list
 * @param delAlbArray Reference to the deleted album array
 * @return CommandAction for the deletion operation
 */
CommandAction createDeleteSingleAlbumCommand(std::shared_ptr<AlbumRemovalState> state, albumList& album, indexSet& delAlbArray)
{
    CommandAction action;
    action.description = "Delete album " + state->album.getTitle();
    action.redo = [&, state]() -> bool {
        std::fstream AlbFile;
        if (!ensureAlbumStream(AlbFile)) {
            return false;
        }
        AlbumFile blank = {"-1", "-1", "", "", "", ""};
        AlbFile.clear();
        AlbFile.seekp(state->pos, ios::beg);
        AlbFile.write(reinterpret_cast<const char*>(&blank), sizeof(AlbumFile));
        AlbFile.flush();
        int albumIdx = findAlbumIndexById(album, state->album.getAlbumId());
        if (albumIdx != -1) {
            album.albList[albumIdx].albumId = "-1";
            album.albList[albumIdx].artistId = "-1";
            album.albList[albumIdx].title = "";
            album.albList[albumIdx].pos = state->pos;
            if (std::find(delAlbArray.indexes.begin(), delAlbArray.indexes.end(), albumIdx) == delAlbArray.indexes.end()) {
                delAlbArray.indexes.push_back(albumIdx);
            }
        }
        Logger::getInstance()->log("Redo delete album: " + state->album.getTitle());
        return true;
    };
    action.undo = [&, state]() {
        std::fstream AlbFile;
        if (!writeAlbumAtPosition(AlbFile, state->pos, state->album)) {
            Logger::getInstance()->log("Failed to restore album during undo");
            return;
        }
        int albumIdx = findAlbumIndexById(album, state->album.getAlbumId());
        if (albumIdx != -1) {
            album.albList[albumIdx].albumId = state->album.getAlbumId();
            album.albList[albumIdx].artistId = state->album.getArtistId();
            album.albList[albumIdx].title = state->album.getTitle();
            album.albList[albumIdx].pos = state->pos;
            auto itAlb = std::find(delAlbArray.indexes.begin(), delAlbArray.indexes.end(), albumIdx);
            if (itAlb != delAlbArray.indexes.end()) {
                delAlbArray.indexes.erase(itAlb);
            }
        }
        Logger::getInstance()->log("Undo delete album: " + state->album.getTitle());
    };
    return action;
}

//69
void deleteAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result, indexSet& delAlbArray)
{
    system("cls");
    cout << setw(30) << "Delete Album " << endl;

    // Select artist for album deletion
    int artistIdx = selectArtistForAlbumDeletion(ArtFile, artist, result);
    if (artistIdx == -1) {
        return;
    }

    // Get confirmation for deleting all albums
    char answer = getDeleteAllAlbumsConfirmation();
    if (answer == 'y' || answer == 'Y') {
        // Delete all albums for the artist
        auto state = prepareAllAlbumsDeletionState(AlbFile, artist, album, result, artistIdx);
        if (state->empty()) {
            cout << "No albums found for this artist." << endl;
            system("pause");
            return;
        }

        CommandAction action = createDeleteAllAlbumsCommand(state, artist, album, delAlbArray, artistIdx);
        if (!executeCommand(std::move(action))) {
            cout << "Failed to remove albums." << endl;
            system("pause");
            return;
        }

        cout << "\n\t All Albums Successfully Removed!\n\n";
        system("pause");
    } else {
        // Delete single album
        auto [albumIdx, state] = prepareSingleAlbumDeletionState(AlbFile, artist, album, result, artistIdx);
        if (albumIdx == -1 || !state) {
            return;
        }

        CommandAction action = createDeleteSingleAlbumCommand(state, album, delAlbArray);
        if (!executeCommand(std::move(action))) {
            cout << "Failed to remove album." << endl;
            system("pause");
            return;
        }

        cout << "\n\t Successfully Removed.\n\n";
        system("pause");
    }
}

//70
void removeAlbum(std::fstream& AlbFile, albumList& album, indexSet& delAlbArray, int idx)
{
    int pos;
    AlbumFile BLANK_ALBUM_FILE = {"-1", "-1", "", "", "", ""};
    if (!AlbFile.is_open()) {
        try {
            openFile(AlbFile, albumFilePath);
        } catch (const FileException& e) {
            cout << e.what() << endl;
            system("pause");
            Logger::getInstance()->log("Failed to open album file for deletion");
            return;
        }
    }
    AlbFile.clear();
    AlbFile.seekp(album.albList[idx].pos, ios::beg);
    pos = AlbFile.tellp();
    AlbFile.write((char*)&BLANK_ALBUM_FILE, sizeof(AlbumFile));
    AlbFile.flush();
    album.albList[idx].albumId = BLANK_ALBUM_FILE.albumIds;
    album.albList[idx].artistId = BLANK_ALBUM_FILE.artistIdRefs;
    album.albList[idx].title = BLANK_ALBUM_FILE.titles;
    album.albList[idx].pos = pos;
    delAlbArray.indexes.push_back(idx);
    cout << "\n\t Successfully Removed.\n\n";
    system("pause");
}

//71
bool searchAlbumByTitle(std::fstream& AlbFile, const albumList& album, indexSet& result, const std::string& title)
{
    result.indexes.clear();
    AlbumFile albFile;
    for (size_t i = 0; i < album.albList.size(); i++) {
        AlbFile.seekg(album.albList[i].pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        std::string albTitle(albFile.titles);
        if (albTitle.find(title) == 0) {
            result.indexes.push_back(i);
        }
    }
    return !result.indexes.empty();
}

bool searchAlbumByDateRange(std::fstream& AlbFile, const albumList& album, indexSet& result, unsigned int startDay, unsigned int startMonth, unsigned int startYear, unsigned int endDay, unsigned int endMonth, unsigned int endYear)
{
    result.indexes.clear();
    AlbumFile albFile;
    for (size_t i = 0; i < album.albList.size(); i++) {
        AlbFile.seekg(album.albList[i].pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        std::string dateStr(albFile.datePublished);
        // Parse date DD/MM/YYYY
        unsigned int day = (dateStr[0] - '0') * 10 + (dateStr[1] - '0');
        unsigned int month = (dateStr[3] - '0') * 10 + (dateStr[4] - '0');
        unsigned int year = (dateStr[6] - '0') * 1000 + (dateStr[7] - '0') * 100 + (dateStr[8] - '0') * 10 + (dateStr[9] - '0');
        if (year > startYear || (year == startYear && (month > startMonth || (month == startMonth && day >= startDay)))) {
            if (year < endYear || (year == endYear && (month < endMonth || (month == endMonth && day <= endDay)))) {
                result.indexes.push_back(i);
            }
        }
    }
    return !result.indexes.empty();
}

void advancedSearchAlbums(std::fstream& AlbFile, const albumList& album, indexSet& result)
{
    system("cls");
    cout << "\nAdvanced Search Options:\n";
    cout << "1. Search by Album Title\n";
    cout << "2. Search by Date Range\n";
    int choice = 0;
    do {
        cout << "Enter choice: ";
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid input. Please enter 1 or 2.\n";
            choice = 0;
            continue;
        }
        cin.ignore(INT_MAX, '\n');
        if (choice != 1 && choice != 2) {
            cout << "Invalid choice. Please enter 1 or 2.\n";
        }
    } while (choice != 1 && choice != 2);

    if (choice == 1) {
        std::string title;
        cout << "Enter album title prefix: ";
        getline(cin, title);
        if (searchAlbumByTitle(AlbFile, album, result, title)) {
            displayAlbumSearchResult(AlbFile, album, result);
        } else {
            printError(4);
        }
    } else if (choice == 2) {
        unsigned int startDay, startMonth, startYear, endDay, endMonth, endYear;
        cout << "Enter start date (DD/MM/YYYY): ";
        while (!(cin >> startDay) || cin.get() != '/' || !(cin >> startMonth) || cin.get() != '/' || !(cin >> startYear)) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid date format. Please enter in DD/MM/YYYY format: ";
        }
        cin.ignore(INT_MAX, '\n');
        cout << "Enter end date (DD/MM/YYYY): ";
        while (!(cin >> endDay) || cin.get() != '/' || !(cin >> endMonth) || cin.get() != '/' || !(cin >> endYear)) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid date format. Please enter in DD/MM/YYYY format: ";
        }
        cin.ignore(INT_MAX, '\n');
        if (searchAlbumByDateRange(AlbFile, album, result, startDay, startMonth, startYear, endDay, endMonth, endYear)) {
            displayAlbumSearchResult(AlbFile, album, result);
        } else {
            printError(4);
        }
    }
    cout << endl << endl;
    system("pause");
}

void farewell() {
    system("cls");
    cout << "\n\n\n\n\n\n\n\t\t\t\tTHANK YOU FOR USING ALBUM MANAGEMENT SYSTEM!\n\n\n\n\n\n\n";
    system("pause");
}

// ArtistManager implementations
bool ArtistManager::load(std::fstream& ArtFile) {
    if (repository) {
        return repository->loadArtists(artists, deletedArtists);
    }
    
    // Fallback to original implementation if no repository
    Logger::getInstance()->log("Loading artists from file");
    ArtistFile artFile;
    int nRec, pos;
    try {
        openFile(ArtFile, artistFilePath);
    } catch(const FileException& e) {
        printError(1);
        system("pause");
        Logger::getInstance()->log("Failed to load artists: " + std::string(e.what()));
        return false;
    }
    ArtFile.seekg(0, ios::end);
    nRec = ArtFile.tellg() / sizeof(artFile);
    artists.artList.reserve(nRec + DEFAULT_SIZE);
    ArtFile.seekg(0, ios::beg);
    pos = 0;
    for (int i = 0; i < nRec; i++){
        ArtFile.read((char*)&artFile, sizeof(artFile));
        if(std::string(artFile.artistIds) != "-1"){
            artists.artList.push_back({std::string(artFile.artistIds), std::string(artFile.names), pos});
            int currentId = stringToInt(std::string(artFile.artistIds));
            if(currentId > lastArtistID){
                lastArtistID = currentId;
            }
        }
        else{
            deletedArtists.indexes.push_back(pos);
        }
        pos = ArtFile.tellg();
    }
    sortArtists();
    Logger::getInstance()->log("Loaded " + std::to_string(artists.artList.size()) + " artists");
    return true;
}

void ArtistManager::sortArtists() {
    std::sort(artists.artList.begin(), artists.artList.end(), [](const artistIndex& a, const artistIndex& b) {
        return a.name < b.name;
    });
}

bool ArtistManager::add(std::fstream& ArtFile) {
    Logger::getInstance()->log("Adding new artist");
    char addA;
    system("cls");
    cout << "Do you want to add an artist? (Y/N) : ";
    cin >> addA;
    cin.ignore(INT_MAX, '\n');
    if (addA == 'y' || addA == 'Y')
    {
        if (!ArtFile.is_open()) {
            try {
                openFile(ArtFile, artistFilePath);
            } catch (const FileException& e) {
                cout << e.what() << endl;
                system("pause");
                return false;
            }
        }

        ArtFile.clear();
        ArtFile.seekp(0, ios::end);

        int pos;
        Artist art = getArtistInfo();
        std::string id = intToString(++lastArtistID, "art");
        art.setArtistId(id);
        ArtistFile artFile{};
        strncpy(artFile.artistIds, art.getArtistId().c_str(), 7);
        artFile.artistIds[7] = '\0';
        strncpy(artFile.names, art.getName().c_str(), 49);
        artFile.names[49] = '\0';
        artFile.genders = art.getGender();
        strncpy(artFile.phones, art.getPhone().c_str(), 14);
        artFile.phones[14] = '\0';
        strncpy(artFile.emails, art.getEmail().c_str(), 49);
        artFile.emails[49] = '\0';
        pos = ArtFile.tellp();
        ArtFile.write((char*)&artFile, sizeof(ArtistFile));
        ArtFile.flush();
        artists.artList.push_back({art.getArtistId(), art.getName(), pos});
        sortArtists();
        Logger::getInstance()->log("Added artist: " + art.getName() + " with ID: " + art.getArtistId());
        return true;
    }else
        return false;
}

void ArtistManager::displayAll(std::fstream& ArtFile) const {
    ArtistView::displayAll(artists);
}

bool ArtistManager::search(std::fstream& ArtFile, indexSet& result) const {
    std::string targetId, targetName;
    int searchby = 0;
    cout << "\t1. Search artist by ID \n\t2. Search artist by Name" << endl;
    do{
        cout << "\t  Choice: ";
        if (!(cin >> searchby)) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid input. Please enter 1 or 2.\n";
            searchby = 0;
            continue;
        }
        cin.ignore(INT_MAX, '\n');
        switch (searchby){
        case 1:
            cout << "\nEnter prefix of Id of Artist: ";
            getline(cin, targetId);
            if (!searchArtistById(artists, result, targetId))
                return false;
            break;
        case 2:
            cout << "\nEnter prefix of name of Artist: ";
            getline(cin, targetName);
            if (!searchArtistByName(artists, result, targetName))
                return false;
            break;
        default:
            cout << "Wrong choice. Enter 1 or 2.\n";
            break;
        }
    }while(searchby != 1 && searchby != 2);

    return true;
}

void ArtistManager::displaySearchResult(std::fstream& ArtFile, const indexSet& result) const {
    ArtistView::displaySearchResult(artists, result);
}

int ArtistManager::selectArtist(std::fstream& ArtFile, indexSet& result, const std::string& forWhat) const {
    int s;
    cout << result.indexes.size() << " results found.\n";
    system("pause");
    if (result.indexes.empty())
        return 0;
    for(size_t i = 0; i < result.indexes.size(); i++)
        cout << '\t' << i+1 << ". " << artists.artList[result.indexes[i]].name << endl;
    cout << "\n\t Select Artist to " << forWhat << ':' ;
    do{
        cin >> s;
        if( s < 1 || s > (int)result.indexes.size() )
             cout << "Wrong choice. Try Again." << endl;
    }while( s < 1 || s > (int)result.indexes.size() );
    return result.indexes[s-1];
}

void ArtistManager::displayOne(std::fstream& ArtFile, int idx) const {
    ArtistView::displayOne(artists, idx);
}

void ArtistManager::edit(std::fstream& ArtFile, indexSet& result) {
    Logger::getInstance()->log("Editing artist");
    system("cls");
    cout << setw(30) << "Edit Artist " << endl;
    do{
        search(ArtFile, result);
        if (result.indexes.empty()){
            printError(4);
            system("pause");
            Logger::getInstance()->log("No artists found for editing");
            return;
        }
    }while(result.indexes.empty());
    size_t idx = selectArtist(ArtFile, result, "edit");
    editArtistInfo(ArtFile, artists, idx);
    sortArtists();
    Logger::getInstance()->log("Artist edited successfully");
}

void ArtistManager::remove(std::fstream& ArtFile, std::fstream& AlbFile, AlbumManager& albumManager, indexSet& result) {
    system("cls");
    cout << setw(30) << "Delete Artist " << endl;
    size_t selectedIdx;
      
       while(result.indexes.empty()){
        search(ArtFile, result);
        selectedIdx = selectArtist(ArtFile, result, "delete");
    }
    displayOne(ArtFile, selectedIdx);
    removeArtist(ArtFile, AlbFile, artists, albumManager.getAlbums(), deletedArtists, albumManager.getDeletedAlbums(), selectedIdx);
}

bool ArtistManager::save(std::fstream& ArtFile) {
    if (repository) {
        return repository->saveArtists(artists, deletedArtists);
    }
    
    // Fallback to original implementation if no repository
    Logger::getInstance()->log("Saving artists to file");
    try {
        openFile(ArtFile, artistFilePath);
    } catch(const FileException& e) {
        printError(1);
        system("pause");
        Logger::getInstance()->log("Failed to save artists: " + std::string(e.what()));
        return false;
    }
    ArtFile.seekp(0, ios::end);
    for (auto& artist : artists.artList) {
        ArtistFile artFile;
        strcpy(artFile.artistIds, artist.artistId.c_str());
        strcpy(artFile.names, artist.name.c_str());
        ArtFile.write((char*)&artFile, sizeof(artFile));
    }
    Logger::getInstance()->log("Saved " + std::to_string(artists.artList.size()) + " artists");
    return true;
}

// AlbumManager implementations
bool AlbumManager::load(std::fstream& AlbFile) {
    Logger::getInstance()->log("Loading albums from file");
    AlbumFile albFile;
    int nRec, pos;

    try {
        openFile(AlbFile, albumFilePath);
    } catch(const FileException& e) {
        printError(2);
        system("pause");
        Logger::getInstance()->log("Failed to load albums: " + std::string(e.what()));
        return false;
    }
    AlbFile.seekg(0, ios::end);
    nRec = AlbFile.tellg() / sizeof(albFile);
    albums.albList.reserve(nRec + DEFAULT_SIZE);
    AlbFile.seekg(0, ios::beg);
    pos = 0;
    for (int i = 0; i < nRec; i++){
        AlbFile.read((char*)&albFile, sizeof(albFile));
        if (std::string(albFile.albumIds) != "-1"){
            albums.albList.push_back(albumIndex{std::string(albFile.albumIds), std::string(albFile.artistIdRefs), std::string(albFile.titles), pos});
            int currentId = stringToInt(std::string(albFile.albumIds));
            if (currentId > lastAlbumID){
                lastAlbumID = currentId;
            }
        }
        else{
            deletedAlbums.indexes.push_back(pos);
        }
        pos = AlbFile.tellg();
    }
    sortAlbums();
    Logger::getInstance()->log("Successfully loaded " + std::to_string(albums.albList.size()) + " albums");
    return true;
}

void AlbumManager::sortAlbums() {
    std::sort(albums.albList.begin(), albums.albList.end(), [](const albumIndex& a, const albumIndex& b) {
        return a.artistId < b.artistId;
    });
}

bool AlbumManager::add(std::fstream& ArtFile, std::fstream& AlbFile, const ArtistManager& artistManager, indexSet& result) {
    Logger::getInstance()->log("Adding new album");
    char addA;
    do{
        system("cls");
        cout << setw(30) << "Add Album " << endl;
        cout << "Do you want to add an album? (Y/N) : ";
        cin >> addA;
        cin.ignore(INT_MAX, '\n');
        if (addA == 'y' || addA == 'Y')
        {
            int pos, select;
            AlbumFile albFile{};
            while(result.indexes.empty()){
                artistManager.search(ArtFile, result);
                if (result.indexes.empty()){
                    printError(4);
                    system("pause");
                }
            }
            select = artistManager.selectArtist(ArtFile, result, "add an album");
            albFile = getAlbumInfo();
            std::string idStr = intToString(++lastAlbumID, "alb");
            strncpy(albFile.albumIds, idStr.c_str(), 7);
            albFile.albumIds[7] = '\0';
            strncpy(albFile.artistIdRefs, artistManager.getArtists().artList[select].artistId.c_str(), 7);
            albFile.artistIdRefs[7] = '\0';
            if (!AlbFile.is_open()) {
                try {
                    openFile(AlbFile, albumFilePath);
                } catch (const FileException& e) {
                    cout << e.what() << endl;
                    system("pause");
                    Logger::getInstance()->log("Failed to open album file for adding");
                    return false;
                }
            }
            AlbFile.clear();
            AlbFile.seekp(0, ios::end);
            pos = AlbFile.tellp();
            AlbFile.write((char*)&albFile, sizeof(albFile));
            AlbFile.flush();
            albums.albList.push_back(albumIndex{std::string(albFile.albumIds), std::string(albFile.artistIdRefs), std::string(albFile.titles), pos});
            sortAlbums();
            cout << endl;
            cout << " Album ID: " << albFile.albumIds << endl;
            cout << endl << endl;
            result.indexes.clear();
            Logger::getInstance()->log("Added album: " + std::string(albFile.titles) + " with ID: " + std::string(albFile.albumIds));
            return true;
        }else if (addA == 'n' || addA == 'N')
            return false;
        else{
            cout << "Wrong entry. Try again!" << endl;
            return false;
        }
    }while(addA != 'y' && addA != 'Y' && addA != 'n' && addA != 'N');
}

void AlbumManager::displayAll(std::fstream& AlbFile) const {
    AlbumView::displayAll(AlbFile, albums);
}

bool AlbumManager::searchByArtistId(std::fstream& AlbFile, indexSet& result, const std::string& targetId) {
    result.indexes.clear();
    for(size_t i = 0; i < albums.albList.size(); i++){
        if(albums.albList[i].artistId != "-1"){
            if(albums.albList[i].artistId.find(targetId) == 0){
               result.indexes.push_back(i);
            }
        }
    }
    return !result.indexes.empty();
}

void AlbumManager::displaySearchResult(std::fstream& AlbFile, const indexSet& result) const {
    AlbumView::displaySearchResult(AlbFile, albums, result);
}

void AlbumManager::displayOne(std::fstream& AlbFile, int idx) const {
    AlbumView::displayOne(AlbFile, albums, idx);
}

int AlbumManager::selectAlbum(std::fstream& AlbFile, const ArtistManager& artistManager, indexSet& result, int idx, const std::string& forWhat) {
    int s;
    result.indexes.clear();
    for(size_t i = 0; i < albums.albList.size(); i++){
        if(albums.albList[i].artistId != "-1"){
            if(artistManager.getArtists().artList[idx].artistId == albums.albList[i].artistId){
               result.indexes.push_back(i);
            }
        }
    }
    if(result.indexes.size() > 0)
        cout << endl << "   " << result.indexes.size() << " albums have been found." << endl << endl;
    else {
        cout << endl << "   The artist has no album. Please add an album first." << endl << endl;
        system("pause");
        return -1;
    }
    system("pause");
    if (result.indexes.size() == 1){
        displayOne(AlbFile, result.indexes[0]);
        return result.indexes[0];
    }
    else if (result.indexes.size() > 1){
        cout << "   Choose an album to " << forWhat << endl;
        for(size_t i = 0; i < result.indexes.size(); i++){
            cout << "       " << i+1 << ". " << albums.albList[result.indexes[i]].title << endl;
        }
        cout << endl;
        do{
            cout << "Enter choice: ";
            if (!(cin >> s)) {
                cin.clear();
                cin.ignore(INT_MAX, '\n');
                cout << "Invalid input. Please enter a number between 1 and " << result.indexes.size() << ".\n";
                s = 0;
                continue;
            }
            cin.ignore(INT_MAX, '\n');
            if( s < 1 || s > (int)result.indexes.size() ){
                cout << "\tError~Choice must be between 1 and " << result.indexes.size() << ".Re-enter." << endl;
                system("pause");
            }
        }while( s < 1 || s > (int)result.indexes.size() );
        return result.indexes[s-1];
    }
    return -1;
}

void AlbumManager::edit(std::fstream& ArtFile, std::fstream& AlbFile, const ArtistManager& artistManager, indexSet& result) {
    Logger::getInstance()->log("Editing album");
    system("cls");
    cout << setw(30) << "Edit Album " << endl;
    int select;
    bool finish = false;
   while(result.indexes.empty()){
        artistManager.search(ArtFile, result);
        select = artistManager.selectArtist(ArtFile, result, "edit");
   }
    select = selectAlbum(AlbFile, artistManager, result, select, "edit");
    if(select == -1 ) {
        Logger::getInstance()->log("Album edit cancelled - no album selected");
        return;
    }
    while (finish == false && !result.indexes.empty())
        finish = editAlbumInfo(AlbFile, albums, select);
    sortAlbums();
    Logger::getInstance()->log("Album edited successfully");
}

void AlbumManager::remove(std::fstream& AlbFile, indexSet& result, int idx) {
    Logger::getInstance()->log("Removing album: " + albums.albList[idx].title + " with ID: " + albums.albList[idx].albumId);
    int pos;
    AlbumFile BLANK_ALBUM_FILE = {"-1", "-1", "", "", "", ""};
    if (!AlbFile.is_open()) {
        try {
            openFile(AlbFile, albumFilePath);
        } catch (const FileException& e) {
            cout << e.what() << endl;
            system("pause");
            Logger::getInstance()->log("Failed to open album file for removal");
            return;
        }
    }
    AlbFile.clear();
    AlbFile.seekp(albums.albList[idx].pos, ios::beg);
    pos = AlbFile.tellp();
    AlbFile.write((char*)&BLANK_ALBUM_FILE, sizeof(AlbumFile));
    AlbFile.flush();
    albums.albList[idx].albumId = BLANK_ALBUM_FILE.albumIds;
    albums.albList[idx].artistId = BLANK_ALBUM_FILE.artistIdRefs;
    albums.albList[idx].title = BLANK_ALBUM_FILE.titles;
    albums.albList[idx].pos = pos;
    deletedAlbums.indexes.push_back(idx);
    cout << "\n\t Successfully Removed.\n\n";
    system("pause");
    Logger::getInstance()->log("Album removed successfully");
}

bool AlbumManager::searchByTitle(std::fstream& AlbFile, indexSet& result, const std::string& title) {
    result.indexes.clear();
    AlbumFile albFile;
    for (size_t i = 0; i < albums.albList.size(); i++) {
        AlbFile.seekg(albums.albList[i].pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        std::string albTitle(albFile.titles);
        if (albTitle.find(title) == 0) {
            result.indexes.push_back(i);
        }
    }
    return !result.indexes.empty();
}

bool AlbumManager::searchByDateRange(std::fstream& AlbFile, indexSet& result, unsigned int startDay, unsigned int startMonth, unsigned int startYear, unsigned int endDay, unsigned int endMonth, unsigned int endYear) {
    result.indexes.clear();
    AlbumFile albFile;
    for (size_t i = 0; i < albums.albList.size(); i++) {
        AlbFile.seekg(albums.albList[i].pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        std::string dateStr(albFile.datePublished);
        // Parse date DD/MM/YYYY
        unsigned int day = (dateStr[0] - '0') * 10 + (dateStr[1] - '0');
        unsigned int month = (dateStr[3] - '0') * 10 + (dateStr[4] - '0');
        unsigned int year = (dateStr[6] - '0') * 1000 + (dateStr[7] - '0') * 100 + (dateStr[8] - '0') * 10 + (dateStr[9] - '0');
        if (year > startYear || (year == startYear && (month > startMonth || (month == startMonth && day >= startDay)))) {
            if (year < endYear || (year == endYear && (month < endMonth || (month == endMonth && day <= endDay)))) {
                result.indexes.push_back(i);
            }
        }
    }
    return !result.indexes.empty();
}

bool AlbumManager::save(std::fstream& AlbFile) {
    if (repository) {
        return repository->saveAlbums(albums, deletedAlbums);
    }
    
    // Fallback to original implementation if no repository
    Logger::getInstance()->log("Saving albums to file");
    try {
        openFile(AlbFile, albumFilePath);
    } catch(const FileException& e) {
        printError(2);
        system("pause");
        Logger::getInstance()->log("Failed to save albums: " + std::string(e.what()));
        return false;
    }
    AlbFile.seekp(0, ios::end);
    for (auto& album : albums.albList) {
        AlbumFile albFile;
        strcpy(albFile.albumIds, album.albumId.c_str());
        strcpy(albFile.artistIdRefs, album.artistId.c_str());
        strcpy(albFile.titles, album.title.c_str());
        AlbFile.write((char*)&albFile, sizeof(albFile));
    }
    Logger::getInstance()->log("Saved " + std::to_string(albums.albList.size()) + " albums");
    return true;
}

// FileHandler implementations
void FileHandler::openFile(std::fstream& fstr, const std::string& path) {
    cout<<'\n';
    
    // Try to open file for reading and writing
    fstr.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!fstr) {
        // File doesn't exist, create it
        std::ofstream createFile(path, std::ios::binary);
        if (!createFile) {
            throw FileException("Failed to create file: " + path);
        }
        createFile.close();
        
        // Now open it for reading and writing
        fstr.open(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!fstr) {
            throw FileException("Failed to open file: " + path);
        }
    }
}

// Repository Implementations
bool FileArtistRepository::loadArtists(artistList& artists, indexSet& deletedArtists) {
    Logger::getInstance()->log("Loading artists from file via repository");
    cout << "Loading artists..." << endl;
    
    fileStream = std::make_unique<std::fstream>();
    try {
        openFile(*fileStream, filePath);
    } catch(const FileException& e) {
        Logger::getInstance()->log("Failed to open artist file: " + std::string(e.what()));
        return false;
    }
    
    ArtistFile artFile;
    int nRec, pos;
    fileStream->seekg(0, std::ios::end);
    nRec = fileStream->tellg() / sizeof(artFile);
    artists.artList.reserve(nRec + DEFAULT_SIZE);
    fileStream->seekg(0, std::ios::beg);
    pos = 0;
    
    for (int i = 0; i < nRec; i++) {
        fileStream->read((char*)&artFile, sizeof(artFile));
        if (std::string(artFile.artistIds) != "-1") {
            artists.artList.push_back({std::string(artFile.artistIds), std::string(artFile.names), pos});
            int currentId = stringToInt(std::string(artFile.artistIds));
            if (currentId > lastArtistID) {
                lastArtistID = currentId;
            }
        } else {
            deletedArtists.indexes.push_back(pos);
        }
        pos = fileStream->tellg();
    }
    
    sortArtist(artists);
    Logger::getInstance()->log("Loaded " + std::to_string(artists.artList.size()) + " artists");
    return true;
}

bool FileArtistRepository::saveArtist(const Artist& artist) {
    if (!fileStream || !fileStream->is_open()) {
        fileStream = std::make_unique<std::fstream>();
        try {
            openFile(*fileStream, filePath);
        } catch(const FileException& e) {
            Logger::getInstance()->log("Failed to open artist file for saving: " + std::string(e.what()));
            return false;
        }
    }
    
    ArtistFile artFile;
    strncpy(artFile.artistIds, artist.getArtistId().c_str(), 7);
    artFile.artistIds[7] = '\0';
    strncpy(artFile.names, artist.getName().c_str(), 49);
    artFile.names[49] = '\0';
    artFile.genders = artist.getGender();
    strncpy(artFile.phones, artist.getPhone().c_str(), 14);
    artFile.phones[14] = '\0';
    strncpy(artFile.emails, artist.getEmail().c_str(), 49);
    artFile.emails[49] = '\0';
    
    fileStream->clear();
    fileStream->seekp(0, std::ios::end);
    fileStream->write((char*)&artFile, sizeof(ArtistFile));
    fileStream->flush();
    
    Logger::getInstance()->log("Saved artist: " + artist.getName());
    return true;
}

bool FileArtistRepository::updateArtist(const Artist& artist, int position) {
    if (!fileStream || !fileStream->is_open()) {
        fileStream = std::make_unique<std::fstream>();
        try {
            openFile(*fileStream, filePath);
        } catch(const FileException& e) {
            Logger::getInstance()->log("Failed to open artist file for updating: " + std::string(e.what()));
            return false;
        }
    }
    
    ArtistFile artFile;
    strncpy(artFile.artistIds, artist.getArtistId().c_str(), 7);
    artFile.artistIds[7] = '\0';
    strncpy(artFile.names, artist.getName().c_str(), 49);
    artFile.names[49] = '\0';
    artFile.genders = artist.getGender();
    strncpy(artFile.phones, artist.getPhone().c_str(), 14);
    artFile.phones[14] = '\0';
    strncpy(artFile.emails, artist.getEmail().c_str(), 49);
    artFile.emails[49] = '\0';
    
    fileStream->clear();
    fileStream->seekp(position, std::ios::beg);
    fileStream->write((char*)&artFile, sizeof(ArtistFile));
    fileStream->flush();
    
    Logger::getInstance()->log("Updated artist: " + artist.getName());
    return true;
}

bool FileArtistRepository::deleteArtist(int position) {
    if (!fileStream || !fileStream->is_open()) {
        fileStream = std::make_unique<std::fstream>();
        try {
            openFile(*fileStream, filePath);
        } catch(const FileException& e) {
            Logger::getInstance()->log("Failed to open artist file for deletion: " + std::string(e.what()));
            return false;
        }
    }
    
    ArtistFile BLANK_ARTIST_FILE = {"-1", "", 'N', "", ""};
    fileStream->clear();
    fileStream->seekp(position, std::ios::beg);
    fileStream->write((char*)&BLANK_ARTIST_FILE, sizeof(ArtistFile));
    fileStream->flush();
    
    Logger::getInstance()->log("Deleted artist at position: " + std::to_string(position));
    return true;
}

bool FileArtistRepository::saveArtists(const artistList& artists, const indexSet& deletedArtists) {
    Logger::getInstance()->log("Saving all artists to file via repository");
    
    fileStream = std::make_unique<std::fstream>();
    try {
        openFile(*fileStream, filePath);
    } catch(const FileException& e) {
        Logger::getInstance()->log("Failed to open artist file for saving: " + std::string(e.what()));
        return false;
    }
    
    // Clear the file and rewrite all artists
    fileStream->close();
    fileStream = std::make_unique<std::fstream>(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    
    for (const auto& artist : artists.artList) {
        ArtistFile artFile;
        strncpy(artFile.artistIds, artist.artistId.c_str(), 7);
        artFile.artistIds[7] = '\0';
        strncpy(artFile.names, artist.name.c_str(), 49);
        artFile.names[49] = '\0';
        // Note: We don't have gender, phone, email in artistIndex, so we'll use default values
        artFile.genders = 'N'; // Default gender
        strcpy(artFile.phones, "");
        strcpy(artFile.emails, "");
        
        fileStream->write((char*)&artFile, sizeof(ArtistFile));
    }
    
    fileStream->close();
    Logger::getInstance()->log("Saved " + std::to_string(artists.artList.size()) + " artists");
    return true;
}

bool FileArtistRepository::searchArtists(const std::string& query, indexSet& results, bool byId) {
    // This would require access to the artist list, so for now we'll return false
    Logger::getInstance()->log("Artist search not implemented in repository yet");
    return false;
}

bool FileAlbumRepository::loadAlbums(albumList& albums, indexSet& deletedAlbums) {
    Logger::getInstance()->log("Loading albums from file via repository");
    cout << "Loading albums..." << endl;
    
    fileStream = std::make_unique<std::fstream>();
    try {
        openFile(*fileStream, filePath);
    } catch(const FileException& e) {
        Logger::getInstance()->log("Failed to open album file: " + std::string(e.what()));
        return false;
    }
    
    AlbumFile albFile;
    int nRec, pos;
    fileStream->seekg(0, std::ios::end);
    nRec = fileStream->tellg() / sizeof(albFile);
    albums.albList.reserve(nRec + DEFAULT_SIZE);
    fileStream->seekg(0, std::ios::beg);
    pos = 0;
    
    for (int i = 0; i < nRec; i++) {
        fileStream->read((char*)&albFile, sizeof(albFile));
        if (std::string(albFile.albumIds) != "-1") {
            albums.albList.push_back(albumIndex{std::string(albFile.albumIds), std::string(albFile.artistIdRefs), std::string(albFile.titles), pos});
            int currentAlbumId = stringToInt(std::string(albFile.albumIds));
            if (currentAlbumId > lastAlbumID) {
                lastAlbumID = currentAlbumId;
            }
        } else {
            deletedAlbums.indexes.push_back(pos);
        }
        pos = fileStream->tellg();
    }
    
    sortAlbum(albums);
    Logger::getInstance()->log("Loaded " + std::to_string(albums.albList.size()) + " albums");
    return true;
}

bool FileAlbumRepository::saveAlbum(const Album& album) {
    if (!fileStream || !fileStream->is_open()) {
        fileStream = std::make_unique<std::fstream>();
        try {
            openFile(*fileStream, filePath);
        } catch(const FileException& e) {
            Logger::getInstance()->log("Failed to open album file for saving: " + std::string(e.what()));
            return false;
        }
    }
    
    AlbumFile albFile;
    strncpy(albFile.albumIds, album.getAlbumId().c_str(), 7);
    albFile.albumIds[7] = '\0';
    strncpy(albFile.artistIdRefs, album.getArtistId().c_str(), 7);
    albFile.artistIdRefs[7] = '\0';
    strncpy(albFile.titles, album.getTitle().c_str(), 79);
    albFile.titles[79] = '\0';
    strcpy(albFile.recordFormats, album.getRecordFormat().c_str());
    strncpy(albFile.datePublished, album.getDatePublished().c_str(), 10);
    albFile.datePublished[10] = '\0';
    strcpy(albFile.paths, album.getPath().c_str());
    
    fileStream->clear();
    fileStream->seekp(0, std::ios::end);
    fileStream->write((char*)&albFile, sizeof(AlbumFile));
    fileStream->flush();
    
    Logger::getInstance()->log("Saved album: " + album.getTitle());
    return true;
}

bool FileAlbumRepository::updateAlbum(const Album& album, int position) {
    if (!fileStream || !fileStream->is_open()) {
        fileStream = std::make_unique<std::fstream>();
        try {
            openFile(*fileStream, filePath);
        } catch(const FileException& e) {
            Logger::getInstance()->log("Failed to open album file for updating: " + std::string(e.what()));
            return false;
        }
    }
    
    AlbumFile albFile;
    strncpy(albFile.albumIds, album.getAlbumId().c_str(), 7);
    albFile.albumIds[7] = '\0';
    strncpy(albFile.artistIdRefs, album.getArtistId().c_str(), 7);
    albFile.artistIdRefs[7] = '\0';
    strncpy(albFile.titles, album.getTitle().c_str(), 79);
    albFile.titles[79] = '\0';
    strcpy(albFile.recordFormats, album.getRecordFormat().c_str());
    strncpy(albFile.datePublished, album.getDatePublished().c_str(), 10);
    albFile.datePublished[10] = '\0';
    strcpy(albFile.paths, album.getPath().c_str());
    
    fileStream->clear();
    fileStream->seekp(position, std::ios::beg);
    fileStream->write((char*)&albFile, sizeof(AlbumFile));
    fileStream->flush();
    
    Logger::getInstance()->log("Updated album: " + album.getTitle());
    return true;
}

bool FileAlbumRepository::deleteAlbum(int position) {
    if (!fileStream || !fileStream->is_open()) {
        fileStream = std::make_unique<std::fstream>();
        try {
            openFile(*fileStream, filePath);
        } catch(const FileException& e) {
            Logger::getInstance()->log("Failed to open album file for deletion: " + std::string(e.what()));
            return false;
        }
    }
    
    AlbumFile BLANK_ALBUM_FILE = {"-1", "-1", "", "", "", ""};
    fileStream->clear();
    fileStream->seekp(position, std::ios::beg);
    fileStream->write((char*)&BLANK_ALBUM_FILE, sizeof(AlbumFile));
    fileStream->flush();
    
    Logger::getInstance()->log("Deleted album at position: " + std::to_string(position));
    return true;
}

bool FileAlbumRepository::searchAlbumsByArtist(const std::string& artistId, indexSet& results) {
    // This would require access to the album list, so for now we'll return false
    Logger::getInstance()->log("Album search by artist not implemented in repository yet");
    return false;
}

bool FileAlbumRepository::searchAlbumsByTitle(const std::string& title, indexSet& results) {
    // This would require access to the album list, so for now we'll return false
    Logger::getInstance()->log("Album search by title not implemented in repository yet");
    return false;
}

bool FileAlbumRepository::searchAlbumsByDateRange(unsigned int startDay, unsigned int startMonth, unsigned int startYear,
                                                unsigned int endDay, unsigned int endMonth, unsigned int endYear, indexSet& results) {
    // This would require access to the album list, so for now we'll return false
    Logger::getInstance()->log("Album search by date range not implemented in repository yet");
    return false;
}

bool FileAlbumRepository::saveAlbums(const albumList& albums, const indexSet& deletedAlbums) {
    Logger::getInstance()->log("Saving all albums to file via repository");
    
    fileStream = std::make_unique<std::fstream>();
    try {
        openFile(*fileStream, filePath);
    } catch(const FileException& e) {
        Logger::getInstance()->log("Failed to open album file for saving: " + std::string(e.what()));
        return false;
    }
    
    // Clear the file and rewrite all albums
    fileStream->close();
    fileStream = std::make_unique<std::fstream>(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    
    for (const auto& album : albums.albList) {
        AlbumFile albFile;
        strncpy(albFile.albumIds, album.albumId.c_str(), 7);
        albFile.albumIds[7] = '\0';
        strncpy(albFile.artistIdRefs, album.artistId.c_str(), 7);
        albFile.artistIdRefs[7] = '\0';
        strncpy(albFile.titles, album.title.c_str(), 79);
        albFile.titles[79] = '\0';
        // Note: We don't have recordFormat, datePublished, path in albumIndex, so we'll use default values
        strcpy(albFile.recordFormats, "mp3");
        strcpy(albFile.datePublished, "01/01/2023");
        strcpy(albFile.paths, "C:\\Music");
        
        fileStream->write((char*)&albFile, sizeof(AlbumFile));
    }
    
    fileStream->close();
    Logger::getInstance()->log("Saved " + std::to_string(albums.albList.size()) + " albums");
    return true;
}

// ArtistView implementations
void ArtistView::displayAll(const artistList& artists) {
    system("cls");
    ArtistFile artFile;
    int idx = 0;
    std::fstream ArtFile("Artist.bin", std::ios::in | std::ios::binary);
    if (!ArtFile.is_open()) {
        cout << "Error opening Artist.bin" << endl;
        system("pause");
        return;
    }
    ArtFile.seekg(0, ios::beg);
    while (ArtFile.read((char*)&artFile, sizeof(artFile))) {
        artFile.artistIds[7] = '\0';
        artFile.names[49] = '\0';
        artFile.phones[14] = '\0';
        artFile.emails[49] = '\0';
        // Sanitize strings to replace non-printable characters with spaces
        for (size_t j = 0; j < sizeof(artFile.names) && artFile.names[j]; ++j) {
            if (!isprint(artFile.names[j])) artFile.names[j] = ' ';
        }
        for (size_t j = 0; j < sizeof(artFile.artistIds) && artFile.artistIds[j]; ++j) {
            if (!isprint(artFile.artistIds[j])) artFile.artistIds[j] = ' ';
        }
        for (size_t j = 0; j < sizeof(artFile.phones) && artFile.phones[j]; ++j) {
            if (!isprint(artFile.phones[j])) artFile.phones[j] = ' ';
        }
        for (size_t j = 0; j < sizeof(artFile.emails) && artFile.emails[j]; ++j) {
            if (!isprint(artFile.emails[j])) artFile.emails[j] = ' ';
        }
        if (!isprint(artFile.genders)) artFile.genders = ' ';
        if (std::string(artFile.artistIds) != "-1") {
    if (idx == 0) {
        cout << left << setw(5) << "No" << setw(10) << "Ids" << setw(25) << "Names" << setw(8) << "Gender" << setw(15) << "Phone" << setw(30) << "Email" << endl;
        cout << string(93, '-') << endl;
    }
    cout << left << setw(5) << ++idx << setw(10) << artFile.artistIds << setw(25) << artFile.names << setw(8) << artFile.genders << setw(15) << artFile.phones << setw(30) << artFile.emails << endl;
        }
    }
    ArtFile.close();
    if(idx == 0){
        system("cls");
        cout << "\nThere is nothing to display.\n" ;
    }
    cout << endl << endl;
    system("pause");
}

void ArtistView::displaySearchResult(const artistList& artists, const indexSet& result) {
    ArtistFile artFile;
    if(result.indexes.empty()){
        printError(4);
        return;
    }else{
        cout << " \tArtist Search Results:" << endl;
        cout << "\t" << result.indexes.size() << " artist found." << endl << endl;
        cout << left << setw(5) << "No" << setw(10) << "Ids" << setw(25) << "Names" << setw(8) << "Gender" << setw(15) << "Phone" << setw(30) << "Email" << endl;
        cout << string(93, '-') << endl;
        std::fstream ArtFile("Artist.bin", std::ios::in | std::ios::binary);
        if (!ArtFile.is_open()) {
            cout << "Error opening Artist.bin" << endl;
            return;
        }
        for (size_t i = 0; i < result.indexes.size(); i++)
        {
            size_t target_idx = result.indexes[i];
            ArtFile.seekg(artists.artList[target_idx].pos, ios::beg);
            ArtFile.read((char*)&artFile, sizeof(ArtistFile));
            artFile.artistIds[7] = '\0';
            artFile.names[49] = '\0';
            artFile.phones[14] = '\0';
            artFile.emails[49] = '\0';
            // Sanitize strings to replace non-printable characters with spaces
            for (size_t j = 0; j < sizeof(artFile.names) && artFile.names[j]; ++j) {
                if (!isprint(artFile.names[j])) artFile.names[j] = ' ';
            }
            for (size_t j = 0; j < sizeof(artFile.artistIds) && artFile.artistIds[j]; ++j) {
                if (!isprint(artFile.artistIds[j])) artFile.artistIds[j] = ' ';
            }
            for (size_t j = 0; j < sizeof(artFile.phones) && artFile.phones[j]; ++j) {
                if (!isprint(artFile.phones[j])) artFile.phones[j] = ' ';
            }
            for (size_t j = 0; j < sizeof(artFile.emails) && artFile.emails[j]; ++j) {
                if (!isprint(artFile.emails[j])) artFile.emails[j] = ' ';
            }
            if (!isprint(artFile.genders)) artFile.genders = ' ';
            cout << left << setw(5) << i+1 << setw(10) << artFile.artistIds << setw(25) << artFile.names << setw(8) << artFile.genders << setw(15) << artFile.phones << setw(30) << artFile.emails << endl;
        }
        ArtFile.close();
    }
}

void ArtistView::displayOne(const artistList& artists, int idx) {
    if (idx < 0 || idx >= static_cast<int>(artists.artList.size())) {
        cout << "Invalid artist selection." << endl;
        system("pause");
        return;
    }

    ArtistFile artFile{};
    std::fstream ArtFile("Artist.bin", std::ios::in | std::ios::binary);
    if (!ArtFile.is_open()) {
        cout << "Error opening Artist.bin" << endl;
        system("pause");
        return;
    }

    ArtFile.seekg(artists.artList[idx].pos, ios::beg);
    if (!ArtFile.read((char*)&artFile, sizeof(artFile))) {
        cout << "Failed to read artist details." << endl;
        system("pause");
        ArtFile.close();
        return;
    }

    artFile.artistIds[7] = '\0';
    artFile.names[49] = '\0';
    artFile.phones[14] = '\0';
    artFile.emails[49] = '\0';
    for (size_t j = 0; j < sizeof(artFile.names) && artFile.names[j]; ++j) {
        if (!isprint(artFile.names[j])) artFile.names[j] = ' ';
    }
    for (size_t j = 0; j < sizeof(artFile.artistIds) && artFile.artistIds[j]; ++j) {
        if (!isprint(artFile.artistIds[j])) artFile.artistIds[j] = ' ';
    }
    for (size_t j = 0; j < sizeof(artFile.phones) && artFile.phones[j]; ++j) {
        if (!isprint(artFile.phones[j])) artFile.phones[j] = ' ';
    }
    for (size_t j = 0; j < sizeof(artFile.emails) && artFile.emails[j]; ++j) {
        if (!isprint(artFile.emails[j])) artFile.emails[j] = ' ';
    }
    if (!isprint(artFile.genders)) artFile.genders = ' ';

    cout << endl << endl;
    cout << "\tId:     " << artFile.artistIds << endl;
    cout << "\tName:   " << artFile.names << endl;
    cout << "\tGender: " << artFile.genders << endl;
    cout << "\tPhone:  " << artFile.phones << endl;
    cout << "\tEmail:  " << artFile.emails << endl;
    cout << endl << endl;
    system("pause");
    ArtFile.close();
}

// AlbumView implementations
void AlbumView::displayAll(std::fstream& AlbFile, const albumList& albums) {
    system("cls");
    AlbumFile albFile;
    int idx = 0;
    cout << endl << "\tNo.\t" << "Titles\t" << setw(33) << "IdsRef" << setw(11) << "\tAlbumIds";
    cout << "\tRecordFormat \t" << "DatePublisheds" << setw(5) << "\tPaths" << endl;
    for (size_t i = 0; i < albums.albList.size(); i++){
        if(albums.albList[i].albumId != "-1"){
            AlbFile.seekg(albums.albList[i].pos, ios::beg);
            AlbFile.read((char*)&albFile, sizeof(albFile));
            albFile.albumIds[7] = '\0';
            albFile.artistIdRefs[7] = '\0';
            albFile.titles[79] = '\0';
            albFile.recordFormats[11] = '\0';
            albFile.datePublished[10] = '\0';
            albFile.paths[99] = '\0';
            // Sanitize strings to replace non-printable characters with spaces
            for (size_t j = 0; j < sizeof(albFile.albumIds) && albFile.albumIds[j]; ++j) {
                if (!isprint(albFile.albumIds[j])) albFile.albumIds[j] = ' ';
            }
            for (size_t j = 0; j < sizeof(albFile.artistIdRefs) && albFile.artistIdRefs[j]; ++j) {
                if (!isprint(albFile.artistIdRefs[j])) albFile.artistIdRefs[j] = ' ';
            }
            for (size_t j = 0; j < sizeof(albFile.titles) && albFile.titles[j]; ++j) {
                if (!isprint(albFile.titles[j])) albFile.titles[j] = ' ';
            }
            for (size_t j = 0; j < sizeof(albFile.recordFormats) && albFile.recordFormats[j]; ++j) {
                if (!isprint(albFile.recordFormats[j])) albFile.recordFormats[j] = ' ';
            }
            for (size_t j = 0; j < sizeof(albFile.datePublished) && albFile.datePublished[j]; ++j) {
                if (!isprint(albFile.datePublished[j])) albFile.datePublished[j] = ' ';
            }
            for (size_t j = 0; j < sizeof(albFile.paths) && albFile.paths[j]; ++j) {
                if (!isprint(albFile.paths[j])) albFile.paths[j] = ' ';
            }
            cout << '\t' << ++idx << '\t' << albFile.titles << ' ' << setw(40 - strlen(albFile.titles)) << albums.albList[i].artistId;
            cout << '\t' << albums.albList[i].albumId << setw(3) << '\t' << '.' << albFile.recordFormats << setw(10) << '\t' << albFile.datePublished << setw(5) << '\t' << albFile.paths << endl;
        }
    }
    cout << endl << endl;
    if(idx == 0)
        cout << "\tNothing to display. Please add an album." << endl;
    system("pause");
}

void AlbumView::displaySearchResult(std::fstream& AlbFile, const albumList& albums, const indexSet& result) {
    cout << endl << " \tAlbum Search Results:" << endl;
    AlbumFile albFile;
    cout << "\t" << result.indexes.size() << " Albums found." << endl << endl;
    cout << endl << "\tNo.\t" << "Titles" << setw(33) << "IdsRef" << setw(11) << "\tAlbumIds";
    cout << "\tRecordFormat \t" << "DatePublisheds" << setw(5) << "\tPaths" << endl;
    for (size_t i = 0; i < result.indexes.size(); i++){
        size_t idx = result.indexes[i];
        AlbFile.seekg(albums.albList[idx].pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        albFile.albumIds[7] = '\0';
        albFile.artistIdRefs[7] = '\0';
        albFile.titles[79] = '\0';
        albFile.recordFormats[11] = '\0';
        albFile.datePublished[10] = '\0';
        albFile.paths[99] = '\0';
        // Sanitize strings to replace non-printable characters with spaces
        for (size_t j = 0; j < sizeof(albFile.albumIds) && albFile.albumIds[j]; ++j) {
            if (!isprint(albFile.albumIds[j])) albFile.albumIds[j] = ' ';
        }
        for (size_t j = 0; j < sizeof(albFile.artistIdRefs) && albFile.artistIdRefs[j]; ++j) {
            if (!isprint(albFile.artistIdRefs[j])) albFile.artistIdRefs[j] = ' ';
        }
        for (size_t j = 0; j < sizeof(albFile.titles) && albFile.titles[j]; ++j) {
            if (!isprint(albFile.titles[j])) albFile.titles[j] = ' ';
        }
        for (size_t j = 0; j < sizeof(albFile.recordFormats) && albFile.recordFormats[j]; ++j) {
            if (!isprint(albFile.recordFormats[j])) albFile.recordFormats[j] = ' ';
        }
        for (size_t j = 0; j < sizeof(albFile.datePublished) && albFile.datePublished[j]; ++j) {
            if (!isprint(albFile.datePublished[j])) albFile.datePublished[j] = ' ';
        }
        for (size_t j = 0; j < sizeof(albFile.paths) && albFile.paths[j]; ++j) {
            if (!isprint(albFile.paths[j])) albFile.paths[j] = ' ';
        }
        cout << '\t' << idx << '\t' << albFile.titles << setw(40 - strlen(albFile.titles)) << albums.albList[idx].artistId;
        cout << '\t' << albums.albList[idx].albumId << setw(3) << '\t' << '.' << albFile.recordFormats << setw(10) << '\t' << albFile.datePublished << setw(5) << '\t' << albFile.paths << endl;
    }
    cout << endl << endl;
}

void AlbumView::displayOne(std::fstream& AlbFile, const albumList& albums, int idx) {
    AlbumFile albFile;
    AlbFile.seekg(albums.albList[idx].pos, ios::beg);
    AlbFile.read((char*)&albFile, sizeof(albFile));
    albFile.albumIds[7] = '\0';
    albFile.artistIdRefs[7] = '\0';
    albFile.titles[79] = '\0';
    albFile.recordFormats[11] = '\0';
    albFile.datePublished[10] = '\0';
    albFile.paths[99] = '\0';
    // Sanitize strings to replace non-printable characters with spaces
    for (size_t j = 0; j < sizeof(albFile.albumIds) && albFile.albumIds[j]; ++j) {
        if (!isprint(albFile.albumIds[j])) albFile.albumIds[j] = ' ';
    }
    for (size_t j = 0; j < sizeof(albFile.artistIdRefs) && albFile.artistIdRefs[j]; ++j) {
        if (!isprint(albFile.artistIdRefs[j])) albFile.artistIdRefs[j] = ' ';
    }
    for (size_t j = 0; j < sizeof(albFile.titles) && albFile.titles[j]; ++j) {
        if (!isprint(albFile.titles[j])) albFile.titles[j] = ' ';
    }
    for (size_t j = 0; j < sizeof(albFile.recordFormats) && albFile.recordFormats[j]; ++j) {
        if (!isprint(albFile.recordFormats[j])) albFile.recordFormats[j] = ' ';
    }
    for (size_t j = 0; j < sizeof(albFile.datePublished) && albFile.datePublished[j]; ++j) {
        if (!isprint(albFile.datePublished[j])) albFile.datePublished[j] = ' ';
    }
    for (size_t j = 0; j < sizeof(albFile.paths) && albFile.paths[j]; ++j) {
        if (!isprint(albFile.paths[j])) albFile.paths[j] = ' ';
    }
    cout << endl << endl;
    cout << "\t\tTitle:          " << albFile.titles << endl;
    cout << "\t\tAlbum ID:       " << albFile.albumIds << endl;
    cout << "\t\tRecord Format:  ." << albFile.recordFormats << endl;
    cout << "\t\tDate Published: " << albFile.datePublished << endl;
    cout << "\t\tPath:           " << albFile.paths << endl;
    cout << endl << endl;
    system("pause");
}

// MenuView implementations
int MenuView::mainMenu() {
    int c;
    do{
        system("COLOR 0A");
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *ALBUM MANAGEMENT SYSTEM*               ";
        cout<<"\n\n                       Enter  1 :  >> ARTIST MANAGER                           ";
        cout<<"\n\n                       Enter  2 :  >> ALBUM MANAGER                            ";
        cout<<"\n\n                       Enter  3 :  >> BACKUP & RESTORE                            ";
        std::string undoDesc = getNextUndoDescription();
        std::string redoDesc = getNextRedoDescription();
        cout<<"\n\n                       Enter  4 :  >> STATISTICS                              ";
        cout<<"\n\n                       Enter  5 :  >> UNDO " << (undoDesc.empty() ? "(none)" : "- " + undoDesc);
        cout<<"\n\n                       Enter  6 :  >> REDO " << (redoDesc.empty() ? "(none)" : "- " + redoDesc);
        cout<<"\n\n                       Enter  7 :  >> EXIT.                              \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>7 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>7 || c<1);
    return c;
}

int MenuView::backupMenu() {
    int c;
    do{
        system("COLOR 1F");
        system("cls");
        cout<<"\n\n";
        cout<<"\n                              *BACKUP & RESTORE MENU*               ";
        cout<<"\n\n                       Enter  1 :  >> Create backup snapshot                ";
        cout<<"\n\n                       Enter  2 :  >> Restore from snapshot                 ";
        cout<<"\n\n                       Enter  3 :  >> Go Back                               \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>3 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>3 || c<1);
    return c;
}

int MenuView::artistMenu() {
    int c;
    do{
        system("COLOR 4E");
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Artist Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> Artist Viewer                           ";
        cout<<"\n\n                       Enter  2 :  >> Artist Editor                            ";
        cout<<"\n\n                       Enter  3 :  >> Export Artists to CSV                              ";
        cout<<"\n\n                       Enter  4 :  >> Go To Main Menu                              ";
        cout<<"\n\n                       Enter  5 :  >> EXIT.                              \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>5 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>5 || c<1);
    return c;
}

int MenuView::viewArtistMenu() {
    int c;
    system("COLOR 4E");
    do{
    system("cls");
    cout<<"\n\n";
    cout<<"\n                                 *View ArtistMenu*               ";
    cout<<"\n\n                       Enter  1 :  >> Display All Artist    ";
    cout<<"\n\n                       Enter  2 :  >> View Artist By Search  ";
    cout<<"\n\n                       Enter  3 :  >> GO BACK.          \n\n ";
    cout<<"\n choice:    ";
    cin>>c;
    cin.clear();
    cin.ignore(INT_MAX,'\n');
    if (c>3 || c<1){
        cout<<"Wrong Choice!";
        cout<<endl<<endl;
        system ("pause");
        system ("cls");
    }
    }while(c>3 || c<1);
    return c;
}

int MenuView::editArtistMenu() {
    int c;
    system("COLOR 2E");
    do{
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Edit Artist Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> ADD Artist    ";
        cout<<"\n\n                       Enter  2 :  >> Edit Artist  ";
        cout<<"\n\n                       Enter  3 :  >> Delete Artist  ";
        cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>4 || c<1){
            cout<<"Wrong Choice!"<<endl;
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
            }
    }while(c>4 || c<1);
    return c;
}

int MenuView::albumMenu() {
    int c;
    do{
        system("COLOR 1B");
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Album Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> Album Viewer                           ";
        cout<<"\n\n                       Enter  2 :  >> Album Editor                            ";
        cout<<"\n\n                       Enter  3 :  >> Export Albums to CSV                              ";
        cout<<"\n\n                       Enter  4 :  >> Go To Main Menu                              ";
        cout<<"\n\n                       Enter  5 :  >> EXIT.                              \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>5 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>5 || c<1);
    return c;
}

int MenuView::viewAlbumMenu() {
    int c;
    system("COLOR 2E");
    do{
    system("cls");
    cout<<"\n\n";
    cout<<"\n                                 *View Album Menu*               ";
    cout<<"\n\n                       Enter  1 :  >> Display All Albums    ";
    cout<<"\n\n                       Enter  2 :  >> View Artist Albums By Search   ";
    cout<<"\n\n                       Enter  3 :  >> Advanced Search   ";
    cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
    cout<<"\n choice:    ";
    cin>>c;
    cin.clear();
    cin.ignore(INT_MAX,'\n');
    if (c>4 || c<1){
        cout<<"Wrong Choice!";
        cout<<endl<<endl;
        system ("pause");
        system ("cls");
    }
    }while(c>4 || c<1);
    return c;
}

int MenuView::editAlbumMenu() {
    int c;
    system("COLOR 2E");
    do{
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Edit Album Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> ADD Album    ";
        cout<<"\n\n                       Enter  2 :  >> Edit Album  ";
        cout<<"\n\n                       Enter  3 :  >> Delete Album  ";
        cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>4 || c<1){
            cout<<"Wrong Choice!"<<endl;
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
            }
    }while(c>4 || c<1);
    return c;
}

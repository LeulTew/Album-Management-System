#ifndef MANAGER_H_INCLUDED
#define MANAGER_H_INCLUDED

#include <string>
#include <vector>
#include <exception>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <memory>
#include <cstring>
#include <climits>

const int DEFAULT_SIZE = 10;
extern int lastArtistID, lastAlbumID;
const std::string artistFilePath = "Artist.bin";
const std::string albumFilePath = "Album.bin";

// Custom Exception Classes
class AlbumManagementException : public std::exception {
private:
    std::string message;
public:
    AlbumManagementException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class FileException : public AlbumManagementException {
public:
    FileException(const std::string& msg) : AlbumManagementException("File Error: " + msg) {}
};

class ValidationException : public AlbumManagementException {
public:
    ValidationException(const std::string& msg) : AlbumManagementException("Validation Error: " + msg) {}
};

class SearchException : public AlbumManagementException {
public:
    SearchException(const std::string& msg) : AlbumManagementException("Search Error: " + msg) {}
};

// Simple Logging Class
class Logger {
private:
    std::ofstream logFile;
    static Logger* instance;
    Logger() {
        logFile.open("album_system.log", std::ios::app);
        if (logFile.is_open()) {
            log("Logger initialized");
        }
    }
public:
    static Logger* getInstance() {
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }
    void log(const std::string& message) {
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            logFile << std::ctime(&time) << ": " << message << std::endl;
        }
    }
    ~Logger() {
        if (logFile.is_open()) {
            log("Logger shutting down");
            logFile.close();
        }
    }
};

//Artist information
struct ArtistFile {
    char artistIds[8];
    char names[50];
    char genders;
    char phones[15];
    char emails[50];
};

class Artist {
private:
    std::string artistId;
    std::string name;
    char gender;
    std::string phone;
    std::string email;
public:
    Artist() = default;
    Artist(const std::string& id, const std::string& n, char g, const std::string& p, const std::string& e)
        : artistId(id), name(n), gender(g), phone(p), email(e) {}
    const std::string& getArtistId() const { return artistId; }
    const std::string& getName() const { return name; }
    char getGender() const { return gender; }
    const std::string& getPhone() const { return phone; }
    const std::string& getEmail() const { return email; }
    void setArtistId(const std::string& id) { artistId = id; }
    void setName(const std::string& n) { name = n; }
    void setGender(char g) { gender = g; }
    void setPhone(const std::string& p) { phone = p; }
    void setEmail(const std::string& e) { email = e; }
};

struct artistIndex {
    std::string artistId; //start from 1000
    std::string name;
    long pos;
};

struct artistList {
    std::vector<artistIndex> artList;
};

//Album information
struct AlbumFile {
    char albumIds[8];
    char artistIdRefs[8];
    char titles[80];
    char recordFormats[12];
    char datePublished[11];
    char paths[100];
};

class Album {
private:
    std::string albumId;
    std::string artistId;
    std::string title;
    std::string recordFormat;
    std::string datePublished;
    std::string path;
public:
    Album() = default;
    Album(const std::string& aid, const std::string& artid, const std::string& t, const std::string& rf, const std::string& dp, const std::string& p)
        : albumId(aid), artistId(artid), title(t), recordFormat(rf), datePublished(dp), path(p) {}
    const std::string& getAlbumId() const { return albumId; }
    const std::string& getArtistId() const { return artistId; }
    const std::string& getTitle() const { return title; }
    const std::string& getRecordFormat() const { return recordFormat; }
    const std::string& getDatePublished() const { return datePublished; }
    const std::string& getPath() const { return path; }
    void setAlbumId(const std::string& aid) { albumId = aid; }
    void setArtistId(const std::string& artid) { artistId = artid; }
    void setTitle(const std::string& t) { title = t; }
    void setRecordFormat(const std::string& rf) { recordFormat = rf; }
    void setDatePublished(const std::string& dp) { datePublished = dp; }
    void setPath(const std::string& p) { path = p; }
};

struct albumIndex {
    std::string albumId; //start from 2000
    std::string artistId; //start from 1000
    std::string title;
    long pos;
};

struct albumList {
    std::vector<albumIndex> albList;
};

struct indexSet {
    std::vector<int> indexes;
};

// Data Persistence Interfaces
class IArtistRepository {
public:
    virtual ~IArtistRepository() = default;
    virtual bool loadArtists(artistList& artists, indexSet& deletedArtists) = 0;
    virtual bool saveArtists(const artistList& artists, const indexSet& deletedArtists) = 0;
    virtual bool saveArtist(const Artist& artist) = 0;
    virtual bool updateArtist(const Artist& artist, int position) = 0;
    virtual bool deleteArtist(int position) = 0;
    virtual bool searchArtists(const std::string& query, indexSet& results, bool byId) = 0;
};

class IAlbumRepository {
public:
    virtual ~IAlbumRepository() = default;
    virtual bool loadAlbums(albumList& albums, indexSet& deletedAlbums) = 0;
    virtual bool saveAlbums(const albumList& albums, const indexSet& deletedAlbums) = 0;
    virtual bool saveAlbum(const Album& album) = 0;
    virtual bool updateAlbum(const Album& album, int position) = 0;
    virtual bool deleteAlbum(int position) = 0;
    virtual bool searchAlbumsByArtist(const std::string& artistId, indexSet& results) = 0;
    virtual bool searchAlbumsByTitle(const std::string& title, indexSet& results) = 0;
    virtual bool searchAlbumsByDateRange(unsigned int startDay, unsigned int startMonth, unsigned int startYear,
                                       unsigned int endDay, unsigned int endMonth, unsigned int endYear, indexSet& results) = 0;
};

class FileHandler;

// View Classes for MVC-like separation
class ArtistView {
public:
    static void displayAll(const artistList& artist);
    static void displaySearchResult(const artistList& artist, const indexSet& result);
    static void displayOne(const artistList& artist, int idx);
    static void displayStatistics(const artistList& artist, const albumList& album);
};

class AlbumView {
public:
    static void displayAll(std::fstream& AlbFile, const albumList& album);
    static void displaySearchResult(std::fstream& AlbFile, const albumList& album, const indexSet& result);
    static void displayOne(std::fstream& AlbFile, const albumList& album, int idx);
};

class MenuView {
public:
    static int mainMenu();
    static int artistMenu();
    static int albumMenu();
    static int viewArtistMenu();
    static int viewAlbumMenu();
    static int editArtistMenu();
    static int editAlbumMenu();
};

//Prototype Declarations
void welcome();
void printError(int errId);
std::string intToString(int last, const std::string& prefix);
int stringToInt(const std::string& arr);
void openFile(std::fstream& fstr, const std::string& path);


bool loading(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtFile, indexSet& delAlbFile);
bool loadArtist(std::fstream& ArtFile, artistList& artist, indexSet& delArtFile);
bool loadAlbum(std::fstream& AlbFile, albumList& album, indexSet& delAlbFile);
void sortArtist(artistList& artist);
void sortAlbum(albumList& album);
void mainH(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);
int mainMenu();
void farewell();

bool artistManager(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);
int artistMenu();
bool artistViewer(std::fstream& ArtFile, const artistList& artist, indexSet& result);
int viewArtistMenu();
void displayAllArtist(std::fstream& ArtFile, const artistList& artist);
void viewArtistBySearch(std::fstream& ArtFile, const artistList& artist, indexSet& result);
bool searchArtist(std::fstream& ArtFile, const artistList& artist, indexSet& result);
bool searchArtistById(const artistList& artist, indexSet& result, const std::string& targetId);
bool searchArtistByName(const artistList& artist, indexSet& result, const std::string& targetName);
void displaySearchResult(std::fstream& ArtFile, const artistList& artist, const indexSet& result);
bool artistEditor(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);
int editArtistMenu();
bool addArtist(std::fstream& ArtFile, artistList& artist);
Artist getArtistInfo();
std::string getArtistName();
char getArtistGender();
std::string getArtistPhone();
std::string getArtistEmail();
void validateName(const std::string& name);
std::string formatName(std::string name);
void validateGender(char gender);
void validatePhone(const std::string& phone);
void validateEmail(const std::string& email);
std::string formatEmail(std::string email);
void editArtist(std::fstream& ArtFile, artistList& artist, indexSet& result);
int selectArtist(std::fstream& ArtFile, const artistList& artist, indexSet& result, const std::string& forWhat);
bool editArtistInfo(std::fstream& ArtFile, artistList& artist, int idx);
void displayOneArtist(std::fstream& ArtFile, const artistList& artist, int idx);
void deleteArtist(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);
void removeArtist(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtArray, indexSet& delAlbArray, int idx);
void removeArtistAllAlbums(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& delAlbArray, int i);

bool albumManager(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);
int albumMenu();
bool albumViewer(std::fstream& AlbFile, const albumList& album, indexSet& result);
int viewAlbumMenu();
void displayAllAlbums(std::fstream& AlbFile, const albumList& album);
bool searchAlbumByArtistId(std::fstream& AlbFile, const albumList& album, indexSet& result, const std::string& targetId);
void displayAlbumSearchResult(std::fstream& AlbFile, const albumList& album, const indexSet& result);
bool albumEditor(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);
int editAlbumMenu();
bool addAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result);
AlbumFile getAlbumInfo();
std::string getAlbumTitle();
std::string getAlbumRecordFormat();
std::string getAlbumDate();
std::string getAlbumPath();
void validateAlbumTitle(const std::string& albumTitle);
std::string formatAlbumTitle(std::string albumTitle);
void validateAlbumFormat(const std::string& albumFormat);
std::string formatAlbumFormat(std::string albumFormat);
void validateAlbumDate(unsigned int day, unsigned int month, unsigned int year);
std::string formatAlbumDate(unsigned int day, unsigned int month, unsigned int year);
void validateAlbumPath(const std::string& albumPath);
std::string formatAlbumPath(std::string albumPath);
void editAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result);
int selectAlbum(std::fstream& AlbFile, const artistList& artist, const albumList& album, indexSet& result, int idx, const std::string& forWhat);
bool editAlbumInfo(std::fstream& AlbFile, albumList& album, int idx);
void displayOneAlbum(std::fstream& AlbFile, const albumList& album, int idx);
void deleteAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result, indexSet& delAlbArray);
void removeAlbum(std::fstream& AlbFile, albumList& album, indexSet& delAlbArray, int idx);

void displayStatistics(const artistList& artist, const albumList& album);

bool searchAlbumByTitle(std::fstream& AlbFile, const albumList& album, indexSet& result, const std::string& title);
bool searchAlbumByDateRange(std::fstream& AlbFile, const albumList& album, indexSet& result, unsigned int startDay, unsigned int startMonth, unsigned int startYear, unsigned int endDay, unsigned int endMonth, unsigned int endYear);
void advancedSearchAlbums(std::fstream& AlbFile, const albumList& album, indexSet& result);

class AlbumManager;

class ArtistManager {
private:
    artistList artists;
    indexSet deletedArtists;
    std::unique_ptr<IArtistRepository> repository;
    
public:
    explicit ArtistManager(std::unique_ptr<IArtistRepository> repo) : repository(std::move(repo)) {}
    ArtistManager() = default;
    bool load(std::fstream& file);
    bool save(std::fstream& file);
    bool add(std::fstream& file);
    void edit(std::fstream& file, indexSet& result);
    void remove(std::fstream& file, std::fstream& albFile, AlbumManager& albumManager, indexSet& result);
    bool search(std::fstream& file, indexSet& result) const;
    void displayAll(std::fstream& file) const;
    void displaySearchResult(std::fstream& file, const indexSet& result) const;
    int selectArtist(std::fstream& file, indexSet& result, const std::string& forWhat) const;
    void displayOne(std::fstream& file, int idx) const;
    const artistList& getArtists() const { return artists; }
    artistList& getArtists() { return artists; }
    const indexSet& getDeletedArtists() const { return deletedArtists; }
    indexSet& getDeletedArtists() { return deletedArtists; }
    void sortArtists();
};

class AlbumManager {
private:
    albumList albums;
    indexSet deletedAlbums;
    std::unique_ptr<IAlbumRepository> repository;
    
public:
    explicit AlbumManager(std::unique_ptr<IAlbumRepository> repo) : repository(std::move(repo)) {}
    AlbumManager() = default;
    bool load(std::fstream& file);
    bool save(std::fstream& file);
    bool add(std::fstream& artFile, std::fstream& albFile, const ArtistManager& artistManager, indexSet& result);
    void edit(std::fstream& artFile, std::fstream& albFile, const ArtistManager& artistManager, indexSet& result);
    void remove(std::fstream& albFile, indexSet& result, int idx);
    bool searchByArtistId(std::fstream& file, indexSet& result, const std::string& targetId);
    bool searchByTitle(std::fstream& file, indexSet& result, const std::string& title);
    bool searchByDateRange(std::fstream& file, indexSet& result, unsigned int startDay, unsigned int startMonth, unsigned int startYear, unsigned int endDay, unsigned int endMonth, unsigned int endYear);
    void displayAll(std::fstream& file) const;
    void displaySearchResult(std::fstream& file, const indexSet& result) const;
    int selectAlbum(std::fstream& file, const ArtistManager& artistManager, indexSet& result, int idx, const std::string& forWhat);
    void displayOne(std::fstream& file, int idx) const;
    const albumList& getAlbums() const { return albums; }
    albumList& getAlbums() { return albums; }
    const indexSet& getDeletedAlbums() const { return deletedAlbums; }
    indexSet& getDeletedAlbums() { return deletedAlbums; }
    void sortAlbums();
};

class FileArtistRepository : public IArtistRepository {
private:
    std::string filePath;
    std::unique_ptr<std::fstream> fileStream;
    
public:
    explicit FileArtistRepository(const std::string& path) : filePath(path) {}
    ~FileArtistRepository() override = default;
    
    bool loadArtists(artistList& artists, indexSet& deletedArtists) override;
    bool saveArtists(const artistList& artists, const indexSet& deletedArtists) override;
    bool saveArtist(const Artist& artist) override;
    bool updateArtist(const Artist& artist, int position) override;
    bool deleteArtist(int position) override;
    bool searchArtists(const std::string& query, indexSet& results, bool byId) override;
};

class FileAlbumRepository : public IAlbumRepository {
private:
    std::string filePath;
    std::unique_ptr<std::fstream> fileStream;
    
public:
    explicit FileAlbumRepository(const std::string& path) : filePath(path) {}
    ~FileAlbumRepository() override = default;
    
    bool loadAlbums(albumList& albums, indexSet& deletedAlbums) override;
    bool saveAlbums(const albumList& albums, const indexSet& deletedAlbums) override;
    bool saveAlbum(const Album& album) override;
    bool updateAlbum(const Album& album, int position) override;
    bool deleteAlbum(int position) override;
    bool searchAlbumsByArtist(const std::string& artistId, indexSet& results) override;
    bool searchAlbumsByTitle(const std::string& title, indexSet& results) override;
    bool searchAlbumsByDateRange(unsigned int startDay, unsigned int startMonth, unsigned int startYear,
                               unsigned int endDay, unsigned int endMonth, unsigned int endYear, indexSet& results) override;
};

class FileHandler {
private:
    std::string artistFilePath = "Artist.bin";
    std::string albumFilePath = "Album.bin";
public:
    FileHandler() = default;
    void openFile(std::fstream& fstr, const std::string& path);
    const std::string& getArtistFilePath() const { return artistFilePath; }
    const std::string& getAlbumFilePath() const { return albumFilePath; }
};

#endif // MANAGER_H_INCLUDED

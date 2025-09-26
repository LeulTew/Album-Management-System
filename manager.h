/**
 * @file manager.h
 * @brief Header file for the Album Management System.
 *
 * This file contains all the class definitions, struct declarations,
 * function prototypes, and constants used in the album management system.
 * It includes exception classes, data structures, repository interfaces,
 * manager classes, and view classes for MVC separation.
 */

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

struct AppConfigSettings {
    std::string artistFile;
    std::string albumFile;
    std::string backupDirectory;
    std::string backupIndexFile;
};

class AppConfig {
public:
    static AppConfig& instance();
    void loadFromFile(const std::string& path);
    const AppConfigSettings& settings() const;
    void resetToDefaults();

private:
    AppConfig();
    void applyDerivedDefaults();
    static std::string extractValue(const std::string& content, const std::string& key);

    AppConfigSettings values;
};

class ConfigValue {
public:
    using Getter = const std::string& (*)();

    constexpr explicit ConfigValue(Getter getter) : getter_(getter) {}

    operator const std::string&() const { return getter_(); }
    const std::string& str() const { return getter_(); }

private:
    Getter getter_;
};

const std::string& getArtistFilePath();
const std::string& getAlbumFilePath();
const std::string& getBackupDirectory();
const std::string& getBackupIndexFile();

extern const ConfigValue artistFilePath;
extern const ConfigValue albumFilePath;
extern const ConfigValue backupDirectory;
extern const ConfigValue backupIndexFile;

void loadApplicationConfig(const std::string& path);

// Custom Exception Classes

/**
 * @brief Base exception class for album management system errors.
 */
class AlbumManagementException : public std::exception {
private:
    std::string message;
public:
    /**
     * @brief Constructs an AlbumManagementException with a message.
     * @param msg The error message.
     */
    AlbumManagementException(const std::string& msg) : message(msg) {}
    /**
     * @brief Returns the error message.
     * @return The error message as a C-string.
     */
    const char* what() const noexcept override { return message.c_str(); }
};

/**
 * @brief Exception class for file-related errors.
 */
class FileException : public AlbumManagementException {
public:
    /**
     * @brief Constructs a FileException with a message.
     * @param msg The error message.
     */
    FileException(const std::string& msg) : AlbumManagementException("File Error: " + msg) {}
};

/**
 * @brief Exception class for validation errors.
 */
class ValidationException : public AlbumManagementException {
public:
    /**
     * @brief Constructs a ValidationException with a message.
     * @param msg The error message.
     */
    ValidationException(const std::string& msg) : AlbumManagementException("Validation Error: " + msg) {}
};

/**
 * @brief Exception class for search-related errors.
 */
class SearchException : public AlbumManagementException {
public:
    /**
     * @brief Constructs a SearchException with a message.
     * @param msg The error message.
     */
    SearchException(const std::string& msg) : AlbumManagementException("Search Error: " + msg) {}
};

// Simple Logging Class

/**
 * @brief Singleton logger class for logging messages to a file.
 */
class Logger {
private:
    std::ofstream logFile;
    static Logger* instance;
    /**
     * @brief Private constructor for singleton pattern.
     */
    Logger() {
        logFile.open("album_system.log", std::ios::app);
        if (logFile.is_open()) {
            log("Logger initialized");
        }
    }
public:
    /**
     * @brief Gets the singleton instance of the logger.
     * @return Pointer to the logger instance.
     */
    static Logger* getInstance() {
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }
    /**
     * @brief Logs a message with timestamp.
     * @param message The message to log.
     */
    void log(const std::string& message) {
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            logFile << std::ctime(&time) << ": " << message << std::endl;
        }
    }
    /**
     * @brief Destructor that logs shutdown and closes the file.
     */
    ~Logger() {
        if (logFile.is_open()) {
            log("Logger shutting down");
            logFile.close();
        }
    }
};

//Artist information

/**
 * @brief Struct representing artist data in binary file format.
 */
struct ArtistFile {
    char artistIds[8]; /**< Artist ID as char array */
    char names[50];    /**< Artist name as char array */
    char genders;      /**< Artist gender as char */
    char phones[15];   /**< Artist phone as char array */
    char emails[50];   /**< Artist email as char array */
};

/**
 * @brief Class representing an Artist with encapsulated data and methods.
 */
class Artist {
private:
    std::string artistId; /**< Unique identifier for the artist */
    std::string name;     /**< Artist's name */
    char gender;          /**< Artist's gender ('M' or 'F') */
    std::string phone;    /**< Artist's phone number */
    std::string email;    /**< Artist's email address */
public:
    /**
     * @brief Default constructor.
     */
    Artist() = default;
    /**
     * @brief Parameterized constructor.
     * @param id Artist ID.
     * @param n Name.
     * @param g Gender.
     * @param p Phone.
     * @param e Email.
     */
    Artist(const std::string& id, const std::string& n, char g, const std::string& p, const std::string& e)
        : artistId(id), name(n), gender(g), phone(p), email(e) {}
    /**
     * @brief Gets the artist ID.
     * @return Artist ID.
     */
    const std::string& getArtistId() const { return artistId; }
    /**
     * @brief Gets the artist's name.
     * @return Artist's name.
     */
    const std::string& getName() const { return name; }
    /**
     * @brief Gets the artist's gender.
     * @return Artist's gender.
     */
    char getGender() const { return gender; }
    /**
     * @brief Gets the artist's phone number.
     * @return Artist's phone.
     */
    const std::string& getPhone() const { return phone; }
    /**
     * @brief Gets the artist's email.
     * @return Artist's email.
     */
    const std::string& getEmail() const { return email; }
    /**
     * @brief Sets the artist ID.
     * @param id New artist ID.
     */
    void setArtistId(const std::string& id) { artistId = id; }
    /**
     * @brief Sets the artist's name.
     * @param n New name.
     */
    void setName(const std::string& n) { name = n; }
    /**
     * @brief Sets the artist's gender.
     * @param g New gender.
     */
    void setGender(char g) { gender = g; }
    /**
     * @brief Sets the artist's phone number.
     * @param p New phone.
     */
    void setPhone(const std::string& p) { phone = p; }
    /**
     * @brief Sets the artist's email.
     * @param e New email.
     */
    void setEmail(const std::string& e) { email = e; }
};

/**
 * @brief Struct representing an artist index entry for quick lookup.
 */
struct artistIndex {
    std::string artistId; /**< Artist ID starting from 1000 */
    std::string name;     /**< Artist name */
    long pos;             /**< Position in file */
};

/**
 * @brief Struct containing a list of artist indices.
 */
struct artistList {
    std::vector<artistIndex> artList; /**< Vector of artist indices */
};

//Album information

/**
 * @brief Struct representing album data in binary file format.
 */
struct AlbumFile {
    char albumIds[8];       /**< Album ID as char array */
    char artistIdRefs[8];   /**< Referenced artist ID as char array */
    char titles[80];        /**< Album title as char array */
    char recordFormats[12]; /**< Record format as char array */
    char datePublished[11]; /**< Publication date as char array */
    char paths[100];        /**< File path as char array */
};

/**
 * @brief Class representing an Album with encapsulated data and methods.
 */
class Album {
private:
    std::string albumId;      /**< Unique identifier for the album */
    std::string artistId;     /**< Referenced artist ID */
    std::string title;        /**< Album title */
    std::string recordFormat; /**< Record format (e.g., CD, Vinyl) */
    std::string datePublished;/**< Publication date in DD/MM/YYYY format */
    std::string path;         /**< File path to album data */
public:
    /**
     * @brief Default constructor.
     */
    Album() = default;
    /**
     * @brief Parameterized constructor.
     * @param aid Album ID.
     * @param artid Artist ID.
     * @param t Title.
     * @param rf Record format.
     * @param dp Date published.
     * @param p Path.
     */
    Album(const std::string& aid, const std::string& artid, const std::string& t, const std::string& rf, const std::string& dp, const std::string& p)
        : albumId(aid), artistId(artid), title(t), recordFormat(rf), datePublished(dp), path(p) {}
    /**
     * @brief Gets the album ID.
     * @return Album ID.
     */
    const std::string& getAlbumId() const { return albumId; }
    /**
     * @brief Gets the referenced artist ID.
     * @return Artist ID.
     */
    const std::string& getArtistId() const { return artistId; }
    /**
     * @brief Gets the album title.
     * @return Album title.
     */
    const std::string& getTitle() const { return title; }
    /**
     * @brief Gets the record format.
     * @return Record format.
     */
    const std::string& getRecordFormat() const { return recordFormat; }
    /**
     * @brief Gets the publication date.
     * @return Publication date.
     */
    const std::string& getDatePublished() const { return datePublished; }
    /**
     * @brief Gets the file path.
     * @return File path.
     */
    const std::string& getPath() const { return path; }
    /**
     * @brief Sets the album ID.
     * @param aid New album ID.
     */
    void setAlbumId(const std::string& aid) { albumId = aid; }
    /**
     * @brief Sets the artist ID.
     * @param artid New artist ID.
     */
    void setArtistId(const std::string& artid) { artistId = artid; }
    /**
     * @brief Sets the album title.
     * @param t New title.
     */
    void setTitle(const std::string& t) { title = t; }
    /**
     * @brief Sets the record format.
     * @param rf New record format.
     */
    void setRecordFormat(const std::string& rf) { recordFormat = rf; }
    /**
     * @brief Sets the publication date.
     * @param dp New date.
     */
    void setDatePublished(const std::string& dp) { datePublished = dp; }
    /**
     * @brief Sets the file path.
     * @param p New path.
     */
    void setPath(const std::string& p) { path = p; }
};

/**
 * @brief Struct representing an album index entry for quick lookup.
 */
struct albumIndex {
    std::string albumId;  /**< Album ID starting from 2000 */
    std::string artistId; /**< Artist ID starting from 1000 */
    std::string title;    /**< Album title */
    long pos;             /**< Position in file */
};

/**
 * @brief Struct containing a list of album indices.
 */
struct albumList {
    std::vector<albumIndex> albList; /**< Vector of album indices */
};

/**
 * @brief Struct containing a set of indices for search results or deletions.
 */
struct indexSet {
    std::vector<int> indexes; /**< Vector of indices */
};

// Data Persistence Interfaces

/**
 * @brief Interface for artist data persistence operations.
 */
class IArtistRepository {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IArtistRepository() = default;
    /**
     * @brief Loads artists from storage.
     * @param artists Reference to artist list to populate.
     * @param deletedArtists Reference to deleted artists index set.
     * @return True if successful.
     */
    virtual bool loadArtists(artistList& artists, indexSet& deletedArtists) = 0;
    /**
     * @brief Saves artists to storage.
     * @param artists Artist list to save.
     * @param deletedArtists Deleted artists index set.
     * @return True if successful.
     */
    virtual bool saveArtists(const artistList& artists, const indexSet& deletedArtists) = 0;
    /**
     * @brief Saves a single artist.
     * @param artist Artist to save.
     * @return True if successful.
     */
    virtual bool saveArtist(const Artist& artist) = 0;
    /**
     * @brief Updates an artist at a specific position.
     * @param artist Updated artist data.
     * @param position Position in storage.
     * @return True if successful.
     */
    virtual bool updateArtist(const Artist& artist, int position) = 0;
    /**
     * @brief Deletes an artist at a specific position.
     * @param position Position to delete.
     * @return True if successful.
     */
    virtual bool deleteArtist(int position) = 0;
    /**
     * @brief Searches for artists by query.
     * @param query Search query.
     * @param results Reference to results index set.
     * @param byId True if searching by ID, false for name.
     * @return True if successful.
     */
    virtual bool searchArtists(const std::string& query, indexSet& results, bool byId) = 0;
};

/**
 * @brief Interface for album data persistence operations.
 */
class IAlbumRepository {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IAlbumRepository() = default;
    /**
     * @brief Loads albums from storage.
     * @param albums Reference to album list to populate.
     * @param deletedAlbums Reference to deleted albums index set.
     * @return True if successful.
     */
    virtual bool loadAlbums(albumList& albums, indexSet& deletedAlbums) = 0;
    /**
     * @brief Saves albums to storage.
     * @param albums Album list to save.
     * @param deletedAlbums Deleted albums index set.
     * @return True if successful.
     */
    virtual bool saveAlbums(const albumList& albums, const indexSet& deletedAlbums) = 0;
    /**
     * @brief Saves a single album.
     * @param album Album to save.
     * @return True if successful.
     */
    virtual bool saveAlbum(const Album& album) = 0;
    /**
     * @brief Updates an album at a specific position.
     * @param album Updated album data.
     * @param position Position in storage.
     * @return True if successful.
     */
    virtual bool updateAlbum(const Album& album, int position) = 0;
    /**
     * @brief Deletes an album at a specific position.
     * @param position Position to delete.
     * @return True if successful.
     */
    virtual bool deleteAlbum(int position) = 0;
    /**
     * @brief Searches albums by artist ID.
     * @param artistId Artist ID to search for.
     * @param results Reference to results index set.
     * @return True if successful.
     */
    virtual bool searchAlbumsByArtist(const std::string& artistId, indexSet& results) = 0;
    /**
     * @brief Searches albums by title.
     * @param title Title to search for.
     * @param results Reference to results index set.
     * @return True if successful.
     */
    virtual bool searchAlbumsByTitle(const std::string& title, indexSet& results) = 0;
    /**
     * @brief Searches albums by date range.
     * @param startDay Start day.
     * @param startMonth Start month.
     * @param startYear Start year.
     * @param endDay End day.
     * @param endMonth End month.
     * @param endYear End year.
     * @param results Reference to results index set.
     * @return True if successful.
     */
    virtual bool searchAlbumsByDateRange(unsigned int startDay, unsigned int startMonth, unsigned int startYear,
                                       unsigned int endDay, unsigned int endMonth, unsigned int endYear, indexSet& results) = 0;
};

class FileHandler;

// View Classes for MVC-like separation

/**
 * @brief Static class for displaying artist-related information.
 */
class ArtistView {
public:
    /**
     * @brief Displays all artists.
     * @param artist Artist list.
     */
    static void displayAll(const artistList& artist);
    /**
     * @brief Displays search results for artists.
     * @param artist Artist list.
     * @param result Index set of results.
     */
    static void displaySearchResult(const artistList& artist, const indexSet& result);
    /**
     * @brief Displays a single artist.
     * @param artist Artist list.
     * @param idx Index of the artist.
     */
    static void displayOne(const artistList& artist, int idx);
    /**
     * @brief Displays statistics for artists and albums.
     * @param artist Artist list.
     * @param album Album list.
     */
    static void displayStatistics(const artistList& artist, const albumList& album);
};

/**
 * @brief Static class for displaying album-related information.
 */
class AlbumView {
public:
    /**
     * @brief Displays all albums.
     * @param AlbFile Album file stream.
     * @param album Album list.
     */
    static void displayAll(std::fstream& AlbFile, const albumList& album);
    /**
     * @brief Displays search results for albums.
     * @param AlbFile Album file stream.
     * @param album Album list.
     * @param result Index set of results.
     */
    static void displaySearchResult(std::fstream& AlbFile, const albumList& album, const indexSet& result);
    /**
     * @brief Displays a single album.
     * @param AlbFile Album file stream.
     * @param album Album list.
     * @param idx Index of the album.
     */
    static void displayOne(std::fstream& AlbFile, const albumList& album, int idx);
};

/**
 * @brief Static class for displaying menus and getting user choices.
 */
class MenuView {
public:
    /**
     * @brief Displays the main menu and gets user choice.
     * @return User's menu choice.
     */
    static int mainMenu();
    /**
     * @brief Displays the artist menu and gets user choice.
     * @return User's menu choice.
     */
    static int artistMenu();
    /**
     * @brief Displays the album menu and gets user choice.
     * @return User's menu choice.
     */
    static int albumMenu();
    /**
     * @brief Displays the view artist menu and gets user choice.
     * @return User's menu choice.
     */
    static int viewArtistMenu();
    /**
     * @brief Displays the view album menu and gets user choice.
     * @return User's menu choice.
     */
    static int viewAlbumMenu();
    /**
     * @brief Displays the edit artist menu and gets user choice.
     * @return User's menu choice.
     */
    static int editArtistMenu();
    /**
     * @brief Displays the edit album menu and gets user choice.
     * @return User's menu choice.
     */
    static int editAlbumMenu();
    /**
     * @brief Displays the backup menu and gets user choice.
     * @return User's menu choice.
     */
    static int backupMenu();
};

//Prototype Declarations

/**
 * @brief Displays the welcome message.
 */
void welcome();

/**
 * @brief Prints an error message based on error ID.
 * @param errId Error identifier.
 */
void printError(int errId);

/**
 * @brief Converts an integer to a string with a prefix.
 * @param last Last used ID.
 * @param prefix Prefix string.
 * @return Formatted string.
 */
std::string intToString(int last, const std::string& prefix);

/**
 * @brief Converts a char array to an integer.
 * @param arr Char array containing the number.
 * @return Integer value.
 */
int stringToInt(const std::string& arr);

/**
 * @brief Opens a file stream for the given path.
 * @param fstr File stream to open.
 * @param path File path.
 */
void openFile(std::fstream& fstr, const std::string& path);


/**
 * @brief Loads data from artist and album files.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list to populate.
 * @param album Album list to populate.
 * @param delArtFile Deleted artist indices.
 * @param delAlbFile Deleted album indices.
 * @return True if successful.
 */
bool loading(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtFile, indexSet& delAlbFile);

/**
 * @brief Loads artist data from file.
 * @param ArtFile Artist file stream.
 * @param artist Artist list to populate.
 * @param delArtFile Deleted artist indices.
 * @return True if successful.
 */
bool loadArtist(std::fstream& ArtFile, artistList& artist, indexSet& delArtFile);

/**
 * @brief Loads album data from file.
 * @param AlbFile Album file stream.
 * @param album Album list to populate.
 * @param delAlbFile Deleted album indices.
 * @return True if successful.
 */
bool loadAlbum(std::fstream& AlbFile, albumList& album, indexSet& delAlbFile);

/**
 * @brief Sorts the artist list.
 * @param artist Artist list to sort.
 */
void sortArtist(artistList& artist);

/**
 * @brief Sorts the album list.
 * @param album Album list to sort.
 */
void sortAlbum(albumList& album);

/**
 * @brief Main handler function for the application.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @param delArtArray Deleted artist indices.
 * @param delAlbArray Deleted album indices.
 */
void mainH(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);

/**
 * @brief Displays the main menu and gets user choice.
 * @return User's menu choice.
 */
int mainMenu();

/**
 * @brief Displays farewell message.
 */
void farewell();

/**
 * @brief Handles artist management operations.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @param delArtArray Deleted artist indices.
 * @param delAlbArray Deleted album indices.
 * @return True if operation successful.
 */
bool artistManager(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);

/**
 * @brief Displays the artist menu and gets user choice.
 * @return User's menu choice.
 */
int artistMenu();

/**
 * @brief Handles artist viewing operations.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @param result Index set for results.
 * @return True if operation successful.
 */
bool artistViewer(std::fstream& ArtFile, const artistList& artist, indexSet& result);

/**
 * @brief Displays the view artist menu and gets user choice.
 * @return User's menu choice.
 */
int viewArtistMenu();

/**
 * @brief Displays all artists.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 */
void displayAllArtist(std::fstream& ArtFile, const artistList& artist);

/**
 * @brief Views artists by search results.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @param result Index set of search results.
 */
void viewArtistBySearch(std::fstream& ArtFile, const artistList& artist, indexSet& result);

/**
 * @brief Searches for artists.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @param result Index set for results.
 * @return True if search successful.
 */
bool searchArtist(std::fstream& ArtFile, const artistList& artist, indexSet& result);

/**
 * @brief Searches artists by ID.
 * @param artist Artist list.
 * @param result Index set for results.
 * @param targetId Target artist ID.
 * @return True if found.
 */
bool searchArtistById(const artistList& artist, indexSet& result, const std::string& targetId);

/**
 * @brief Searches artists by name.
 * @param artist Artist list.
 * @param result Index set for results.
 * @param targetName Target artist name.
 * @return True if found.
 */
bool searchArtistByName(const artistList& artist, indexSet& result, const std::string& targetName);

/**
 * @brief Displays search results for artists.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @param result Index set of results.
 */
void displaySearchResult(std::fstream& ArtFile, const artistList& artist, const indexSet& result);

/**
 * @brief Handles artist editing operations.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @param delArtArray Deleted artist indices.
 * @param delAlbArray Deleted album indices.
 * @return True if operation successful.
 */
bool artistEditor(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);

/**
 * @brief Displays the edit artist menu and gets user choice.
 * @return User's menu choice.
 */
int editArtistMenu();

/**
 * @brief Adds a new artist.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @return True if added successfully.
 */
bool addArtist(std::fstream& ArtFile, artistList& artist);

/**
 * @brief Gets artist information from user input.
 * @return Artist object with user input.
 */
Artist getArtistInfo();

/**
 * @brief Gets artist name from user input.
 * @return Artist name.
 */
std::string getArtistName();

/**
 * @brief Gets artist gender from user input.
 * @return Artist gender.
 */
char getArtistGender();

/**
 * @brief Gets artist phone from user input.
 * @return Artist phone.
 */
std::string getArtistPhone();

/**
 * @brief Gets artist email from user input.
 * @return Artist email.
 */
std::string getArtistEmail();
/**
 * @brief Validates artist name.
 * @param name Name to validate.
 * @throws ValidationException if invalid.
 */
void validateName(const std::string& name);

/**
 * @brief Formats artist name.
 * @param name Name to format.
 * @return Formatted name.
 */
std::string formatName(std::string name);

/**
 * @brief Validates gender.
 * @param gender Gender to validate.
 * @throws ValidationException if invalid.
 */
void validateGender(char gender);

/**
 * @brief Validates phone number.
 * @param phone Phone to validate.
 * @throws ValidationException if invalid.
 */
void validatePhone(const std::string& phone);

/**
 * @brief Validates email address.
 * @param email Email to validate.
 * @throws ValidationException if invalid.
 */
void validateEmail(const std::string& email);

/**
 * @brief Formats email address.
 * @param email Email to format.
 * @return Formatted email.
 */
std::string formatEmail(std::string email);

/**
 * @brief Edits an artist.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @param result Index set for selection.
 */
void editArtist(std::fstream& ArtFile, artistList& artist, indexSet& result);

/**
 * @brief Selects an artist for editing.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @param result Index set for results.
 * @param forWhat Purpose description.
 * @return Selected index.
 */
int selectArtist(std::fstream& ArtFile, const artistList& artist, indexSet& result, const std::string& forWhat);

/**
 * @brief Edits artist information.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @param idx Index of artist to edit.
 * @return True if edited successfully.
 */
bool editArtistInfo(std::fstream& ArtFile, artistList& artist, int idx);

/**
 * @brief Displays a single artist.
 * @param ArtFile Artist file stream.
 * @param artist Artist list.
 * @param idx Index of artist.
 */
void displayOneArtist(std::fstream& ArtFile, const artistList& artist, int idx);

/**
 * @brief Deletes an artist and associated albums.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @param delArtArray Deleted artist indices.
 * @param delAlbArray Deleted album indices.
 */
void deleteArtist(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);

/**
 * @brief Removes an artist from storage.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param delArtArray Deleted artist indices.
 * @param delAlbArray Deleted album indices.
 * @param idx Index to remove.
 */
void removeArtist(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtArray, indexSet& delAlbArray, int idx);

/**
 * @brief Removes all albums of an artist.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param delAlbArray Deleted album indices.
 * @param i Artist index.
 */
/**
 * @brief Removes all albums of an artist.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param delAlbArray Deleted album indices.
 * @param i Artist index.
 */
void removeArtistAllAlbums(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& delAlbArray, int i);

/**
 * @brief Handles backup and restore operations.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param delArtArray Deleted artist indices.
 * @param delAlbArray Deleted album indices.
 */
void backupAndRestoreMenu(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtArray, indexSet& delAlbArray);

/**
 * @brief Handles album management operations.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @param delArtArray Deleted artist indices.
 * @param delAlbArray Deleted album indices.
 * @return True if operation successful.
 */
bool albumManager(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);

/**
 * @brief Displays the album menu and gets user choice.
 * @return User's menu choice.
 */
int albumMenu();

/**
 * @brief Handles album viewing operations.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param result Index set for results.
 * @return True if operation successful.
 */
bool albumViewer(std::fstream& AlbFile, const albumList& album, indexSet& result);

/**
 * @brief Displays the view album menu and gets user choice.
 * @return User's menu choice.
 */
int viewAlbumMenu();

/**
 * @brief Displays all albums.
 * @param AlbFile Album file stream.
 * @param album Album list.
 */
void displayAllAlbums(std::fstream& AlbFile, const albumList& album);

/**
 * @brief Searches albums by artist ID.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param result Index set for results.
 * @param targetId Target artist ID.
 * @return True if found.
 */
bool searchAlbumByArtistId(std::fstream& AlbFile, const albumList& album, indexSet& result, const std::string& targetId);

/**
 * @brief Displays album search results.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param result Index set of results.
 */
void displayAlbumSearchResult(std::fstream& AlbFile, const albumList& album, const indexSet& result);
/**
 * @brief Handles album editing operations.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @param delArtArray Deleted artist indices.
 * @param delAlbArray Deleted album indices.
 * @return True if operation successful.
 */
bool albumEditor(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray);

/**
 * @brief Displays the edit album menu and gets user choice.
 * @return User's menu choice.
 */
int editAlbumMenu();

/**
 * @brief Adds a new album.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @return True if added successfully.
 */
bool addAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result);

/**
 * @brief Gets album information from user input.
 * @return AlbumFile struct with user input.
 */
AlbumFile getAlbumInfo();

/**
 * @brief Gets album title from user input.
 * @return Album title.
 */
std::string getAlbumTitle();

/**
 * @brief Gets album record format from user input.
 * @return Record format.
 */
std::string getAlbumRecordFormat();

/**
 * @brief Gets album publication date from user input.
 * @return Publication date.
 */
std::string getAlbumDate();

/**
 * @brief Gets album path from user input.
 * @return File path.
 */
std::string getAlbumPath();
/**
 * @brief Validates album title.
 * @param albumTitle Title to validate.
 * @throws ValidationException if invalid.
 */
void validateAlbumTitle(const std::string& albumTitle);

/**
 * @brief Formats album title.
 * @param albumTitle Title to format.
 * @return Formatted title.
 */
std::string formatAlbumTitle(std::string albumTitle);

/**
 * @brief Validates album format.
 * @param albumFormat Format to validate.
 * @throws ValidationException if invalid.
 */
void validateAlbumFormat(const std::string& albumFormat);

/**
 * @brief Formats album format.
 * @param albumFormat Format to format.
 * @return Formatted format.
 */
std::string formatAlbumFormat(std::string albumFormat);

/**
 * @brief Validates album date.
 * @param day Day.
 * @param month Month.
 * @param year Year.
 * @throws ValidationException if invalid.
 */
void validateAlbumDate(unsigned int day, unsigned int month, unsigned int year);

/**
 * @brief Formats album date.
 * @param day Day.
 * @param month Month.
 * @param year Year.
 * @return Formatted date.
 */
std::string formatAlbumDate(unsigned int day, unsigned int month, unsigned int year);

/**
 * @brief Validates album path.
 * @param albumPath Path to validate.
 * @throws ValidationException if invalid.
 */
void validateAlbumPath(const std::string& albumPath);

/**
 * @brief Formats album path.
 * @param albumPath Path to format.
 * @return Formatted path.
 */
std::string formatAlbumPath(std::string albumPath);
/**
 * @brief Edits an album.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for selection.
 */
void editAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result);

/**
 * @brief Selects an album for editing.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @param idx Current index.
 * @param forWhat Purpose description.
 * @return Selected index.
 */
int selectAlbum(std::fstream& AlbFile, const artistList& artist, const albumList& album, indexSet& result, int idx, const std::string& forWhat);

/**
 * @brief Edits album information.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param idx Index of album.
 * @return True if edited successfully.
 */
bool editAlbumInfo(std::fstream& AlbFile, albumList& album, int idx);

/**
 * @brief Displays a single album.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param idx Index of album.
 */
void displayOneAlbum(std::fstream& AlbFile, const albumList& album, int idx);

/**
 * @brief Deletes an album.
 * @param ArtFile Artist file stream.
 * @param AlbFile Album file stream.
 * @param artist Artist list.
 * @param album Album list.
 * @param result Index set for results.
 * @param delAlbArray Deleted album indices.
 */
void deleteAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result, indexSet& delAlbArray);

/**
 * @brief Removes an album from storage.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param delAlbArray Deleted album indices.
 * @param idx Index to remove.
 */
void removeAlbum(std::fstream& AlbFile, albumList& album, indexSet& delAlbArray, int idx);

/**
 * @brief Displays statistics for artists and albums.
 * @param artist Artist list.
 * @param album Album list.
 */
void displayStatistics(const artistList& artist, const albumList& album);

/**
 * @brief Searches albums by title.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param result Index set for results.
 * @param title Title to search.
 * @return True if found.
 */
bool searchAlbumByTitle(std::fstream& AlbFile, const albumList& album, indexSet& result, const std::string& title);

/**
 * @brief Searches albums by date range.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param result Index set for results.
 * @param startDay Start day.
 * @param startMonth Start month.
 * @param startYear Start year.
 * @param endDay End day.
 * @param endMonth End month.
 * @param endYear End year.
 * @return True if found.
 */
bool searchAlbumByDateRange(std::fstream& AlbFile, const albumList& album, indexSet& result, unsigned int startDay, unsigned int startMonth, unsigned int startYear, unsigned int endDay, unsigned int endMonth, unsigned int endYear);

/**
 * @brief Performs advanced search on albums.
 * @param AlbFile Album file stream.
 * @param album Album list.
 * @param result Index set for results.
 */
void advancedSearchAlbums(std::fstream& AlbFile, const albumList& album, indexSet& result);

// Command pattern helpers

/**
 * @brief Undoes the last action.
 * @return True if undo successful.
 */
bool undoLastAction();

/**
 * @brief Redoes the last undone action.
 * @return True if redo successful.
 */
bool redoLastAction();

/**
 * @brief Checks if undo is possible.
 * @return True if can undo.
 */
bool canUndo();

/**
 * @brief Checks if redo is possible.
 * @return True if can redo.
 */
bool canRedo();

/**
 * @brief Gets description of next undo action.
 * @return Undo description.
 */
std::string getNextUndoDescription();

/**
 * @brief Gets description of next redo action.
 * @return Redo description.
 */
std::string getNextRedoDescription();

class AlbumManager;

/**
 * @brief Manager class for handling artist operations.
 */
class ArtistManager {
private:
    artistList artists; /**< List of artists */
    indexSet deletedArtists; /**< Set of deleted artist indices */
    std::unique_ptr<IArtistRepository> repository; /**< Repository for persistence */
    
public:
    /**
     * @brief Constructs ArtistManager with a repository.
     * @param repo Unique pointer to artist repository.
     */
    explicit ArtistManager(std::unique_ptr<IArtistRepository> repo) : repository(std::move(repo)) {}
    /**
     * @brief Default constructor.
     */
    ArtistManager() = default;
    /**
     * @brief Loads artists from file.
     * @param file File stream.
     * @return True if successful.
     */
    bool load(std::fstream& file);
    /**
     * @brief Saves artists to file.
     * @param file File stream.
     * @return True if successful.
     */
    bool save(std::fstream& file);
    /**
     * @brief Adds a new artist.
     * @param file File stream.
     * @return True if successful.
     */
    bool add(std::fstream& file);
    /**
     * @brief Edits an artist.
     * @param file File stream.
     * @param result Index set for selection.
     */
    void edit(std::fstream& file, indexSet& result);
    /**
     * @brief Removes an artist and associated albums.
     * @param file File stream.
     * @param albFile Album file stream.
     * @param albumManager Reference to album manager.
     * @param result Index set for selection.
     */
    void remove(std::fstream& file, std::fstream& albFile, AlbumManager& albumManager, indexSet& result);
    /**
     * @brief Searches for artists.
     * @param file File stream.
     * @param result Index set for results.
     * @return True if successful.
     */
    bool search(std::fstream& file, indexSet& result) const;
    /**
     * @brief Displays all artists.
     * @param file File stream.
     */
    void displayAll(std::fstream& file) const;
    /**
     * @brief Displays search results.
     * @param file File stream.
     * @param result Index set of results.
     */
    void displaySearchResult(std::fstream& file, const indexSet& result) const;
    /**
     * @brief Selects an artist for editing.
     * @param file File stream.
     * @param result Index set for selection.
     * @param forWhat Purpose description.
     * @return Selected index.
     */
    int selectArtist(std::fstream& file, indexSet& result, const std::string& forWhat) const;
    /**
     * @brief Displays a single artist.
     * @param file File stream.
     * @param idx Index of artist.
     */
    void displayOne(std::fstream& file, int idx) const;
    /**
     * @brief Gets the artist list.
     * @return Reference to artist list.
     */
    const artistList& getArtists() const { return artists; }
    /**
     * @brief Gets the mutable artist list.
     * @return Reference to artist list.
     */
    artistList& getArtists() { return artists; }
    /**
     * @brief Gets the deleted artists set.
     * @return Reference to deleted artists.
     */
    const indexSet& getDeletedArtists() const { return deletedArtists; }
    /**
     * @brief Gets the mutable deleted artists set.
     * @return Reference to deleted artists.
     */
    indexSet& getDeletedArtists() { return deletedArtists; }
    /**
     * @brief Sorts the artists.
     */
    void sortArtists();
};

/**
 * @brief Manager class for handling album operations.
 */
class AlbumManager {
private:
    albumList albums; /**< List of albums */
    indexSet deletedAlbums; /**< Set of deleted album indices */
    std::unique_ptr<IAlbumRepository> repository; /**< Repository for persistence */
    
public:
    /**
     * @brief Constructs AlbumManager with a repository.
     * @param repo Unique pointer to album repository.
     */
    explicit AlbumManager(std::unique_ptr<IAlbumRepository> repo) : repository(std::move(repo)) {}
    /**
     * @brief Default constructor.
     */
    AlbumManager() = default;
    /**
     * @brief Loads albums from file.
     * @param file File stream.
     * @return True if successful.
     */
    bool load(std::fstream& file);
    /**
     * @brief Saves albums to file.
     * @param file File stream.
     * @return True if successful.
     */
    bool save(std::fstream& file);
    /**
     * @brief Adds a new album.
     * @param artFile Artist file stream.
     * @param albFile Album file stream.
     * @param artistManager Reference to artist manager.
     * @param result Index set for selection.
     * @return True if successful.
     */
    bool add(std::fstream& artFile, std::fstream& albFile, const ArtistManager& artistManager, indexSet& result);
    /**
     * @brief Edits an album.
     * @param artFile Artist file stream.
     * @param albFile Album file stream.
     * @param artistManager Reference to artist manager.
     * @param result Index set for selection.
     */
    void edit(std::fstream& artFile, std::fstream& albFile, const ArtistManager& artistManager, indexSet& result);
    /**
     * @brief Removes an album.
     * @param albFile Album file stream.
     * @param result Index set for selection.
     * @param idx Index to remove.
     */
    void remove(std::fstream& albFile, indexSet& result, int idx);
    /**
     * @brief Searches albums by artist ID.
     * @param file File stream.
     * @param result Index set for results.
     * @param targetId Artist ID to search.
     * @return True if successful.
     */
    bool searchByArtistId(std::fstream& file, indexSet& result, const std::string& targetId);
    /**
     * @brief Searches albums by title.
     * @param file File stream.
     * @param result Index set for results.
     * @param title Title to search.
     * @return True if successful.
     */
    bool searchByTitle(std::fstream& file, indexSet& result, const std::string& title);
    /**
     * @brief Searches albums by date range.
     * @param file File stream.
     * @param result Index set for results.
     * @param startDay Start day.
     * @param startMonth Start month.
     * @param startYear Start year.
     * @param endDay End day.
     * @param endMonth End month.
     * @param endYear End year.
     * @return True if successful.
     */
    bool searchByDateRange(std::fstream& file, indexSet& result, unsigned int startDay, unsigned int startMonth, unsigned int startYear, unsigned int endDay, unsigned int endMonth, unsigned int endYear);
    /**
     * @brief Displays all albums.
     * @param file File stream.
     */
    void displayAll(std::fstream& file) const;
    /**
     * @brief Displays search results.
     * @param file File stream.
     * @param result Index set of results.
     */
    void displaySearchResult(std::fstream& file, const indexSet& result) const;
    /**
     * @brief Selects an album for editing.
     * @param file File stream.
     * @param artistManager Reference to artist manager.
     * @param result Index set for selection.
     * @param idx Current index.
     * @param forWhat Purpose description.
     * @return Selected index.
     */
    int selectAlbum(std::fstream& file, const ArtistManager& artistManager, indexSet& result, int idx, const std::string& forWhat);
    /**
     * @brief Displays a single album.
     * @param file File stream.
     * @param idx Index of album.
     */
    void displayOne(std::fstream& file, int idx) const;
    /**
     * @brief Gets the album list.
     * @return Reference to album list.
     */
    const albumList& getAlbums() const { return albums; }
    /**
     * @brief Gets the mutable album list.
     * @return Reference to album list.
     */
    albumList& getAlbums() { return albums; }
    /**
     * @brief Gets the deleted albums set.
     * @return Reference to deleted albums.
     */
    const indexSet& getDeletedAlbums() const { return deletedAlbums; }
    /**
     * @brief Gets the mutable deleted albums set.
     * @return Reference to deleted albums.
     */
    indexSet& getDeletedAlbums() { return deletedAlbums; }
    /**
     * @brief Sorts the albums.
     */
    void sortAlbums();
};

/**
 * @brief File-based implementation of IArtistRepository.
 */
class FileArtistRepository : public IArtistRepository {
private:
    std::string filePath; /**< Path to the artist file */
    std::unique_ptr<std::fstream> fileStream; /**< File stream for operations */
    
public:
    /**
     * @brief Constructs FileArtistRepository with file path.
     * @param path Path to artist file.
     */
    explicit FileArtistRepository(const std::string& path) : filePath(path) {}
    /**
     * @brief Destructor.
     */
    ~FileArtistRepository() override = default;
    
    bool loadArtists(artistList& artists, indexSet& deletedArtists) override;
    bool saveArtists(const artistList& artists, const indexSet& deletedArtists) override;
    bool saveArtist(const Artist& artist) override;
    bool updateArtist(const Artist& artist, int position) override;
    bool deleteArtist(int position) override;
    bool searchArtists(const std::string& query, indexSet& results, bool byId) override;
};

/**
 * @brief File-based implementation of IAlbumRepository.
 */
class FileAlbumRepository : public IAlbumRepository {
private:
    std::string filePath; /**< Path to the album file */
    std::unique_ptr<std::fstream> fileStream; /**< File stream for operations */
    
public:
    /**
     * @brief Constructs FileAlbumRepository with file path.
     * @param path Path to album file.
     */
    explicit FileAlbumRepository(const std::string& path) : filePath(path) {}
    /**
     * @brief Destructor.
     */
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

/**
 * @brief Utility class for handling file operations.
 */
class FileHandler {
private:
    std::string artistFilePath = "Artist.bin"; /**< Default artist file path */
    std::string albumFilePath = "Album.bin";   /**< Default album file path */
public:
    /**
     * @brief Default constructor.
     */
    FileHandler() = default;
    /**
     * @brief Opens a file stream.
     * @param fstr File stream to open.
     * @param path Path to the file.
     */
    void openFile(std::fstream& fstr, const std::string& path);
    /**
     * @brief Gets the artist file path.
     * @return Artist file path.
     */
    const std::string& getArtistFilePath() const { return artistFilePath; }
    /**
     * @brief Gets the album file path.
     * @return Album file path.
     */
    const std::string& getAlbumFilePath() const { return albumFilePath; }
};

#endif // MANAGER_H_INCLUDED

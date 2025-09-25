/**
 * @file main.cpp
 * @brief Main entry point for the Album Management System.
 *
 * This file contains the main function that initializes the application,
 * sets up repositories and managers, loads data, and starts the main menu loop.
 */

#include <iostream>
using namespace std;
#include <conio.h>
#include <fstream>
#include "manager.h"

/**
 * @brief Main entry point of the application.
 * @return Exit status (0 for success).
 */
int main()
{
    // Create repository instances
    auto artistRepo = std::make_unique<FileArtistRepository>(artistFilePath);
    auto albumRepo = std::make_unique<FileAlbumRepository>(albumFilePath);
    
    // Create manager instances with repositories
    ArtistManager artistManager(std::move(artistRepo));
    AlbumManager albumManager(std::move(albumRepo));
    
    fstream ArtFile;
    fstream AlbFile;
    indexSet result = {};
    indexSet delArtArray={}, delAlbArray={};

    welcome();
    
    // Load data using managers
    if(!artistManager.load(ArtFile) || !albumManager.load(AlbFile))
        return 0;
        
    // Convert to old format for compatibility with existing functions
    artistList artist = artistManager.getArtists();
    albumList album = albumManager.getAlbums();
    indexSet delArtFile = artistManager.getDeletedArtists();
    indexSet delAlbFile = albumManager.getDeletedAlbums();
    
    mainH(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
    farewell();
    return 0;
}

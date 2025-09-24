#include <fstream>
#include <iostream>
#include <cstring>
#include <string>

// Struct definitions matching the main program
struct ArtistFile {
    char artistIds[8];
    char names[50];
    char genders;
    char phones[15];
    char emails[50];
};

struct AlbumFile {
    char albumIds[8];
    char artistIdRefs[8];
    char titles[80];
    char recordFormats[12];
    char datePublished[11];
    char paths[100];
};

int main() {
    std::cout << "Testing Artist.bin data integrity:" << std::endl;
    
    std::ifstream artFile("Artist.bin", std::ios::binary);
    if (artFile.is_open()) {
        ArtistFile artist;
        while (artFile.read((char*)&artist, sizeof(ArtistFile))) {
            // Apply null termination like in the fixed code
            artist.artistIds[7] = '\0';
            artist.names[49] = '\0';
            artist.phones[14] = '\0';
            artist.emails[49] = '\0';
            
            std::cout << "ID: " << artist.artistIds << std::endl;
            std::cout << "Name: " << artist.names << std::endl;
            std::cout << "Gender: " << artist.genders << std::endl;
            std::cout << "Phone: " << artist.phones << std::endl;
            std::cout << "Email: " << artist.emails << std::endl;
            std::cout << "---" << std::endl;
        }
        artFile.close();
    } else {
        std::cout << "Error opening Artist.bin" << std::endl;
    }

    std::cout << "\nTesting Album.bin data integrity:" << std::endl;
    
    std::ifstream albFile("Album.bin", std::ios::binary);
    if (albFile.is_open()) {
        AlbumFile album;
        while (albFile.read((char*)&album, sizeof(AlbumFile))) {
            // Apply null termination like in the fixed code
            album.albumIds[7] = '\0';
            album.artistIdRefs[7] = '\0';
            album.titles[79] = '\0';
            album.recordFormats[11] = '\0';
            album.datePublished[10] = '\0';
            album.paths[99] = '\0';
            
            std::cout << "Album ID: " << album.albumIds << std::endl;
            std::cout << "Artist ID: " << album.artistIdRefs << std::endl;
            std::cout << "Title: " << album.titles << std::endl;
            std::cout << "Format: " << album.recordFormats << std::endl;
            std::cout << "Date: " << album.datePublished << std::endl;
            std::cout << "Path: " << album.paths << std::endl;
            std::cout << "---" << std::endl;
        }
        albFile.close();
    } else {
        std::cout << "Error opening Album.bin" << std::endl;
    }

    return 0;
}
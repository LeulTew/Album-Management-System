#include <fstream>
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
    // Sample artists
    ArtistFile artists[] = {
        {"art1000", "John Smith", 'M', "1234567890", "john.smith@email.com"},
        {"art1001", "Sarah Johnson", 'F', "0987654321", "sarah.j@email.com"},
        {"art1002", "Mike Davis", 'M', "5551234567", "mike.davis@email.com"},
        {"art1003", "Emma Wilson", 'F', "7778889999", "emma.wilson@email.com"},
        {"art1004", "David Brown", 'M', "4445556666", "david.brown@email.com"}
    };

    // Sample albums
    AlbumFile albums[] = {
        {"alb2000", "art1000", "Greatest Hits", "mp3", "15/03/2020", "C:\\Music\\JohnSmith\\GreatestHits"},
        {"alb2001", "art1000", "Live Concert", "flac", "22/07/2021", "C:\\Music\\JohnSmith\\LiveConcert"},
        {"alb2002", "art1001", "Pop Collection", "m4a", "10/11/2019", "C:\\Music\\SarahJohnson\\PopCollection"},
        {"alb2003", "art1002", "Rock Anthems", "wav", "05/09/2022", "C:\\Music\\MikeDavis\\RockAnthems"},
        {"alb2004", "art1003", "Jazz Standards", "aac", "18/12/2020", "C:\\Music\\EmmaWilson\\JazzStandards"},
        {"alb2005", "art1004", "Blues Classics", "mp3", "30/01/2023", "C:\\Music\\DavidBrown\\BluesClassics"},
        {"alb2006", "art1001", "Acoustic Sessions", "flac", "14/06/2021", "C:\\Music\\SarahJohnson\\AcousticSessions"}
    };

    // Write artists to file
    std::ofstream artFile("Artist.bin", std::ios::binary);
    if (artFile.is_open()) {
        for (const auto& artist : artists) {
            artFile.write((char*)&artist, sizeof(ArtistFile));
        }
        artFile.close();
        std::cout << "Sample artists added to Artist.bin" << std::endl;
    }

    // Write albums to file
    std::ofstream albFile("Album.bin", std::ios::binary);
    if (albFile.is_open()) {
        for (const auto& album : albums) {
            albFile.write((char*)&album, sizeof(AlbumFile));
        }
        albFile.close();
        std::cout << "Sample albums added to Album.bin" << std::endl;
    }

    return 0;
}
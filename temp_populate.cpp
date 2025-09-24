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
    // Sample artists
    ArtistFile artists[5];
    
    // Initialize artists with proper null termination
    strcpy(artists[0].artistIds, "art1000");
    strcpy(artists[0].names, "John Smith");
    artists[0].genders = 'M';
    strcpy(artists[0].phones, "1234567890");
    strcpy(artists[0].emails, "john.smith@email.com");
    
    strcpy(artists[1].artistIds, "art1001");
    strcpy(artists[1].names, "Sarah Johnson");
    artists[1].genders = 'F';
    strcpy(artists[1].phones, "0987654321");
    strcpy(artists[1].emails, "sarah.j@email.com");
    
    strcpy(artists[2].artistIds, "art1002");
    strcpy(artists[2].names, "Mike Davis");
    artists[2].genders = 'M';
    strcpy(artists[2].phones, "5551234567");
    strcpy(artists[2].emails, "mike.davis@email.com");
    
    strcpy(artists[3].artistIds, "art1003");
    strcpy(artists[3].names, "Emma Wilson");
    artists[3].genders = 'F';
    strcpy(artists[3].phones, "7778889999");
    strcpy(artists[3].emails, "emma.wilson@email.com");
    
    strcpy(artists[4].artistIds, "art1004");
    strcpy(artists[4].names, "David Brown");
    artists[4].genders = 'M';
    strcpy(artists[4].phones, "4445556666");
    strcpy(artists[4].emails, "david.brown@email.com");

    // Sample albums
    AlbumFile albums[7];
    
    // Initialize albums with proper null termination
    strcpy(albums[0].albumIds, "alb2000");
    strcpy(albums[0].artistIdRefs, "art1000");
    strcpy(albums[0].titles, "Greatest Hits");
    strcpy(albums[0].recordFormats, "mp3");
    strcpy(albums[0].datePublished, "15/03/2020");
    strcpy(albums[0].paths, "C:\\Music\\JohnSmith\\GreatestHits");
    
    strcpy(albums[1].albumIds, "alb2001");
    strcpy(albums[1].artistIdRefs, "art1000");
    strcpy(albums[1].titles, "Live Concert");
    strcpy(albums[1].recordFormats, "flac");
    strcpy(albums[1].datePublished, "22/07/2021");
    strcpy(albums[1].paths, "C:\\Music\\JohnSmith\\LiveConcert");
    
    strcpy(albums[2].albumIds, "alb2002");
    strcpy(albums[2].artistIdRefs, "art1001");
    strcpy(albums[2].titles, "Pop Collection");
    strcpy(albums[2].recordFormats, "m4a");
    strcpy(albums[2].datePublished, "10/11/2019");
    strcpy(albums[2].paths, "C:\\Music\\SarahJohnson\\PopCollection");
    
    strcpy(albums[3].albumIds, "alb2003");
    strcpy(albums[3].artistIdRefs, "art1002");
    strcpy(albums[3].titles, "Rock Anthems");
    strcpy(albums[3].recordFormats, "wav");
    strcpy(albums[3].datePublished, "05/09/2022");
    strcpy(albums[3].paths, "C:\\Music\\MikeDavis\\RockAnthems");
    
    strcpy(albums[4].albumIds, "alb2004");
    strcpy(albums[4].artistIdRefs, "art1003");
    strcpy(albums[4].titles, "Jazz Standards");
    strcpy(albums[4].recordFormats, "aac");
    strcpy(albums[4].datePublished, "18/12/2020");
    strcpy(albums[4].paths, "C:\\Music\\EmmaWilson\\JazzStandards");
    
    strcpy(albums[5].albumIds, "alb2005");
    strcpy(albums[5].artistIdRefs, "art1004");
    strcpy(albums[5].titles, "Blues Classics");
    strcpy(albums[5].recordFormats, "mp3");
    strcpy(albums[5].datePublished, "30/01/2023");
    strcpy(albums[5].paths, "C:\\Music\\DavidBrown\\BluesClassics");
    
    strcpy(albums[6].albumIds, "alb2006");
    strcpy(albums[6].artistIdRefs, "art1001");
    strcpy(albums[6].titles, "Acoustic Sessions");
    strcpy(albums[6].recordFormats, "flac");
    strcpy(albums[6].datePublished, "14/06/2021");
    strcpy(albums[6].paths, "C:\\Music\\SarahJohnson\\AcousticSessions");

    // Write artists to file (overwrite)
    std::ofstream artFile("Artist.bin", std::ios::binary | std::ios::trunc);
    if (artFile.is_open()) {
        for (const auto& artist : artists) {
            artFile.write((char*)&artist, sizeof(ArtistFile));
        }
        artFile.close();
        std::cout << "Sample artists written to Artist.bin" << std::endl;
    } else {
        std::cout << "Error opening Artist.bin" << std::endl;
    }

    // Write albums to file (overwrite)
    std::ofstream albFile("Album.bin", std::ios::binary | std::ios::trunc);
    if (albFile.is_open()) {
        for (const auto& album : albums) {
            albFile.write((char*)&album, sizeof(AlbumFile));
        }
        albFile.close();
        std::cout << "Sample albums written to Album.bin" << std::endl;
    } else {
        std::cout << "Error opening Album.bin" << std::endl;
    }

    return 0;
}
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "manager.h"

TEST(IntegrationTest, AddArtistAndPersist) {
    // Use temporary files
    std::string tempArtistFile = "temp_artist_integration.bin";
    std::string tempAlbumFile = "temp_album_integration.bin";

    // Clean up any existing files
    std::filesystem::remove(tempArtistFile);
    std::filesystem::remove(tempAlbumFile);

    // Create repositories
    auto artistRepo = std::make_unique<FileArtistRepository>(tempArtistFile);
    auto albumRepo = std::make_unique<FileAlbumRepository>(tempAlbumFile);

    ArtistManager artistManager(std::move(artistRepo));
    AlbumManager albumManager(std::move(albumRepo));

    // Create a test artist
    Artist testArtist("art1000", "Integration Test Artist", 'M', "1234567890", "integration@example.com");

    // Simulate saving artist
    std::fstream artFile;
    openFile(artFile, tempArtistFile);
    artistRepo = std::make_unique<FileArtistRepository>(tempArtistFile); // Recreate since moved
    EXPECT_TRUE(artistRepo->saveArtist(testArtist));

    // Load and verify
    artistList artists;
    indexSet deleted;
    EXPECT_TRUE(artistRepo->loadArtists(artists, deleted));
    EXPECT_EQ(artists.artList.size(), 1);
    EXPECT_EQ(artists.artList[0].artistId, "art1000");
    EXPECT_EQ(artists.artList[0].name, "Integration Test Artist");

    // Clean up
    artFile.close();
    std::filesystem::remove(tempArtistFile);
    std::filesystem::remove(tempAlbumFile);
}

TEST(IntegrationTest, AddAlbumAndPersist) {
    // Similar for album
    std::string tempArtistFile = "temp_artist_integration2.bin";
    std::string tempAlbumFile = "temp_album_integration2.bin";

    std::filesystem::remove(tempArtistFile);
    std::filesystem::remove(tempAlbumFile);

    auto albumRepo = std::make_unique<FileAlbumRepository>(tempAlbumFile);

    Album testAlbum("alb2000", "art1000", "Integration Test Album", "mp3", "01/01/2023", "C:\\Music");

    EXPECT_TRUE(albumRepo->saveAlbum(testAlbum));

    albumList albums;
    indexSet deleted;
    EXPECT_TRUE(albumRepo->loadAlbums(albums, deleted));
    EXPECT_EQ(albums.albList.size(), 1);
    EXPECT_EQ(albums.albList[0].albumId, "alb2000");
    EXPECT_EQ(albums.albList[0].title, "Integration Test Album");

    std::filesystem::remove(tempArtistFile);
    std::filesystem::remove(tempAlbumFile);
}
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "manager.h"

class RepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary files for testing
        tempArtistFile = "temp_artist.bin";
        tempAlbumFile = "temp_album.bin";
    }

    void TearDown() override {
        // Clean up temporary files
        std::filesystem::remove(tempArtistFile);
        std::filesystem::remove(tempAlbumFile);
    }

    std::string tempArtistFile;
    std::string tempAlbumFile;
};

TEST_F(RepositoryTest, FileArtistRepository_SaveAndLoad) {
    FileArtistRepository repo(tempArtistFile);

    Artist artist("art1000", "Test Artist", 'M', "1234567890", "test@example.com");

    // Save artist
    EXPECT_TRUE(repo.saveArtist(artist));

    // Load artists
    artistList artists;
    indexSet deleted;
    EXPECT_TRUE(repo.loadArtists(artists, deleted));

    EXPECT_EQ(artists.artList.size(), 1);
    EXPECT_EQ(artists.artList[0].artistId, "art1000");
    EXPECT_EQ(artists.artList[0].name, "Test Artist");
}

TEST_F(RepositoryTest, FileAlbumRepository_SaveAndLoad) {
    FileAlbumRepository repo(tempAlbumFile);

    Album album("alb2000", "art1000", "Test Album", "mp3", "01/01/2023", "C:\\Music");

    // Save album
    EXPECT_TRUE(repo.saveAlbum(album));

    // Load albums
    albumList albums;
    indexSet deleted;
    EXPECT_TRUE(repo.loadAlbums(albums, deleted));

    EXPECT_EQ(albums.albList.size(), 1);
    EXPECT_EQ(albums.albList[0].albumId, "alb2000");
    EXPECT_EQ(albums.albList[0].title, "Test Album");
}
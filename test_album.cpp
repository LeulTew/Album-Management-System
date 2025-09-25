#include <gtest/gtest.h>
#include "manager.h"

TEST(AlbumTest, ConstructorAndGetters) {
    Album album("alb2000", "art1000", "Greatest Hits", "mp3", "01/01/2023", "C:\\Music");

    EXPECT_EQ(album.getAlbumId(), "alb2000");
    EXPECT_EQ(album.getArtistId(), "art1000");
    EXPECT_EQ(album.getTitle(), "Greatest Hits");
    EXPECT_EQ(album.getRecordFormat(), "mp3");
    EXPECT_EQ(album.getDatePublished(), "01/01/2023");
    EXPECT_EQ(album.getPath(), "C:\\Music");
}

TEST(AlbumTest, Setters) {
    Album album;
    album.setAlbumId("alb2001");
    album.setArtistId("art1001");
    album.setTitle("New Album");
    album.setRecordFormat("flac");
    album.setDatePublished("02/02/2023");
    album.setPath("D:\\Music");

    EXPECT_EQ(album.getAlbumId(), "alb2001");
    EXPECT_EQ(album.getArtistId(), "art1001");
    EXPECT_EQ(album.getTitle(), "New Album");
    EXPECT_EQ(album.getRecordFormat(), "flac");
    EXPECT_EQ(album.getDatePublished(), "02/02/2023");
    EXPECT_EQ(album.getPath(), "D:\\Music");
}

TEST(AlbumTest, DefaultConstructor) {
    Album album;
    EXPECT_EQ(album.getAlbumId(), "");
    EXPECT_EQ(album.getArtistId(), "");
    EXPECT_EQ(album.getTitle(), "");
    EXPECT_EQ(album.getRecordFormat(), "");
    EXPECT_EQ(album.getDatePublished(), "");
    EXPECT_EQ(album.getPath(), "");
}
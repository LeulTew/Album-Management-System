#include <gtest/gtest.h>
#include "manager.h"

TEST(ArtistTest, ConstructorAndGetters) {
    Artist artist("art1000", "John Doe", 'M', "1234567890", "john@example.com");

    EXPECT_EQ(artist.getArtistId(), "art1000");
    EXPECT_EQ(artist.getName(), "John Doe");
    EXPECT_EQ(artist.getGender(), 'M');
    EXPECT_EQ(artist.getPhone(), "1234567890");
    EXPECT_EQ(artist.getEmail(), "john@example.com");
}

TEST(ArtistTest, Setters) {
    Artist artist;
    artist.setArtistId("art1001");
    artist.setName("Jane Doe");
    artist.setGender('F');
    artist.setPhone("0987654321");
    artist.setEmail("jane@example.com");

    EXPECT_EQ(artist.getArtistId(), "art1001");
    EXPECT_EQ(artist.getName(), "Jane Doe");
    EXPECT_EQ(artist.getGender(), 'F');
    EXPECT_EQ(artist.getPhone(), "0987654321");
    EXPECT_EQ(artist.getEmail(), "jane@example.com");
}

TEST(ArtistTest, DefaultConstructor) {
    Artist artist;
    EXPECT_EQ(artist.getArtistId(), "");
    EXPECT_EQ(artist.getName(), "");
    EXPECT_EQ(artist.getGender(), '\0');
    EXPECT_EQ(artist.getPhone(), "");
    EXPECT_EQ(artist.getEmail(), "");
}
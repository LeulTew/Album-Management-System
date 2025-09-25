#include <gtest/gtest.h>
#include "manager.h"

TEST(ValidationTest, ValidateName_Valid) {
    EXPECT_NO_THROW(validateName("John Doe"));
    EXPECT_NO_THROW(validateName("Jane"));
}

TEST(ValidationTest, ValidateName_Invalid) {
    EXPECT_THROW(validateName(""), ValidationException);
    EXPECT_THROW(validateName(" John"), ValidationException); // starts with space
    EXPECT_THROW(validateName("John@"), ValidationException); // invalid char
}

TEST(ValidationTest, ValidateEmail_Valid) {
    EXPECT_NO_THROW(validateEmail("john@example.com"));
    EXPECT_NO_THROW(validateEmail("jane@example"));
}

TEST(ValidationTest, ValidateEmail_Invalid) {
    EXPECT_THROW(validateEmail(""), ValidationException);
    EXPECT_THROW(validateEmail(" john@example.com"), ValidationException); // starts with space
    EXPECT_THROW(validateEmail("johnexample.com"), ValidationException); // no @
    EXPECT_THROW(validateEmail("john@"), ValidationException); // invalid domain
}

TEST(ValidationTest, ValidatePhone_Valid) {
    EXPECT_NO_THROW(validatePhone("1234567890"));
    EXPECT_NO_THROW(validatePhone("123456789012345"));
}

TEST(ValidationTest, ValidatePhone_Invalid) {
    EXPECT_THROW(validatePhone(""), ValidationException);
    EXPECT_THROW(validatePhone("123"), ValidationException); // too short
    EXPECT_THROW(validatePhone("1234567890123456"), ValidationException); // too long
    EXPECT_THROW(validatePhone("123456789a"), ValidationException); // non-digit
}

TEST(ValidationTest, ValidateGender_Valid) {
    EXPECT_NO_THROW(validateGender('M'));
    EXPECT_NO_THROW(validateGender('F'));
}

TEST(ValidationTest, ValidateGender_Invalid) {
    EXPECT_THROW(validateGender('X'), ValidationException);
    EXPECT_THROW(validateGender('m'), ValidationException); // lowercase
}

TEST(ValidationTest, ValidateAlbumTitle_Valid) {
    EXPECT_NO_THROW(validateAlbumTitle("Greatest Hits"));
    EXPECT_NO_THROW(validateAlbumTitle("Album"));
}

TEST(ValidationTest, ValidateAlbumTitle_Invalid) {
    EXPECT_THROW(validateAlbumTitle(""), ValidationException);
    EXPECT_THROW(validateAlbumTitle(" Album"), ValidationException); // starts with space
    EXPECT_THROW(validateAlbumTitle("Album@"), ValidationException); // invalid char
}

TEST(ValidationTest, ValidateAlbumPath_Valid) {
    EXPECT_NO_THROW(validateAlbumPath("C:\\Music"));
    EXPECT_NO_THROW(validateAlbumPath("D:\\Folder\\Sub"));
}

TEST(ValidationTest, ValidateAlbumPath_Invalid) {
    EXPECT_THROW(validateAlbumPath(""), ValidationException);
    EXPECT_THROW(validateAlbumPath(" Music"), ValidationException); // starts with space
    EXPECT_THROW(validateAlbumPath("Music"), ValidationException); // no backslash
}
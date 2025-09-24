# Black Box Test Cases for Album Management System

## Overview
This document contains comprehensive black box test cases for the Album Management System. Each test case focuses on user-visible behavior without considering internal implementation details.

## Test Case Format
- **Test Case ID**: Unique identifier
- **Test Scenario**: Description of what is being tested
- **Preconditions**: Required state before test
- **Test Steps**: Step-by-step actions
- **Expected Result**: What should happen
- **Actual Result**: To be filled during testing
- **Pass/Fail**: Test outcome

## Main Menu Tests

### TC-MM-001: Main Menu Display
**Test Scenario**: Verify main menu displays correctly with all options
**Preconditions**: Application started
**Test Steps**:
1. Launch the application
2. Observe the main menu
**Expected Result**: Menu shows "ARTIST MANAGER", "ALBUM MANAGER", "EXIT" options
**Actual Result**:
**Pass/Fail**:

### TC-MM-002: Artist Manager Selection
**Test Scenario**: Verify selecting Artist Manager navigates correctly
**Preconditions**: Main menu displayed
**Test Steps**:
1. Enter "1" for Artist Manager
**Expected Result**: Artist Menu displays with Viewer, Editor, Export, Main Menu, Exit options
**Actual Result**:
**Pass/Fail**:

### TC-MM-003: Album Manager Selection
**Test Scenario**: Verify selecting Album Manager navigates correctly
**Preconditions**: Main menu displayed
**Test Steps**:
1. Enter "2" for Album Manager
**Expected Result**: Album Menu displays with Viewer, Editor, Export, Main Menu, Exit options
**Actual Result**:
**Pass/Fail**:

### TC-MM-004: Exit Application
**Test Scenario**: Verify exit option shows statistics and terminates application
**Preconditions**: Main menu displayed
**Test Steps**:
1. Enter "4" for Exit
**Expected Result**: Statistics displayed, then application terminates with farewell message
**Actual Result**:
**Pass/Fail**:

### TC-MM-005: Invalid Main Menu Input
**Test Scenario**: Verify handling of invalid menu choices
**Preconditions**: Main menu displayed
**Test Steps**:
1. Enter "5" (invalid choice)
**Expected Result**: Error message "Wrong Choice!" displayed, menu redisplays
**Actual Result**:
**Pass/Fail**:

### TC-MM-006: Statistics Display
**Test Scenario**: Verify statistics option displays correct information
**Preconditions**: Main menu displayed, data exists
**Test Steps**:
1. Enter "3" for Statistics
**Expected Result**: Statistics screen shows total artists, total albums, and albums per artist
**Actual Result**:
**Pass/Fail**:

### TC-MM-007: Submenu Exit with Statistics
**Test Scenario**: Verify exiting from Artist Manager shows statistics
**Preconditions**: Artist Menu displayed
**Test Steps**:
1. Enter "5" for EXIT
**Expected Result**: Statistics displayed, then application terminates with farewell message
**Actual Result**:
**Pass/Fail**:

### TC-MM-008: Album Submenu Exit with Statistics
**Test Scenario**: Verify exiting from Album Manager shows statistics
**Preconditions**: Album Menu displayed
**Test Steps**:
1. Enter "5" for EXIT
**Expected Result**: Statistics displayed, then application terminates with farewell message
**Actual Result**:
**Pass/Fail**:

## Artist Manager Tests

### TC-AM-001: Artist Viewer - Display All (Empty)
**Test Scenario**: Verify display all artists when no artists exist
**Preconditions**: Artist Menu displayed, no artists in system
**Test Steps**:
1. Select "1" Artist Viewer
2. Select "1" Display All Artist
**Expected Result**: Message "There is nothing to display." shown
**Actual Result**:
**Pass/Fail**:

### TC-AM-002: Artist Viewer - Display All (With Data)
**Test Scenario**: Verify display all artists with existing data
**Preconditions**: Artist Menu displayed, artists exist in system
**Test Steps**:
1. Select "1" Artist Viewer
2. Select "1" Display All Artist
**Expected Result**: Table showing artist details: No, Name, Artist ID, Gender, Phone, Email
**Actual Result**:
**Pass/Fail**:

### TC-AM-003: Artist Search - By ID (Found)
**Test Scenario**: Verify artist search by ID when match exists
**Preconditions**: Artist Menu displayed, artists exist
**Test Steps**:
1. Select "1" Artist Viewer
2. Select "2" View Artist By Search
3. Choose "1" Search by ID
4. Enter valid artist ID prefix
**Expected Result**: Search results displayed with matching artists
**Actual Result**:
**Pass/Fail**:

### TC-AM-004: Artist Search - By ID (Not Found)
**Test Scenario**: Verify artist search by ID when no match exists
**Preconditions**: Artist Menu displayed, artists exist
**Test Steps**:
1. Select "1" Artist Viewer
2. Select "2" View Artist By Search
3. Choose "1" Search by ID
4. Enter non-existent artist ID
**Expected Result**: Error message "No results found" or similar
**Actual Result**:
**Pass/Fail**:

### TC-AM-005: Artist Search - By Name (Found)
**Test Scenario**: Verify artist search by name when match exists
**Preconditions**: Artist Menu displayed, artists exist
**Test Steps**:
1. Select "1" Artist Viewer
2. Select "2" View Artist By Search
3. Choose "2" Search by Name
4. Enter valid artist name prefix
**Expected Result**: Search results displayed with matching artists
**Actual Result**:
**Pass/Fail**:

### TC-AM-006: Add Artist - Valid Data
**Test Scenario**: Verify adding artist with valid information
**Preconditions**: Artist Menu displayed
**Test Steps**:
1. Select "2" Artist Editor
2. Select "1" ADD Artist
3. Choose "Y" to add
4. Enter valid name, gender, phone, email
**Expected Result**: Artist added successfully, ID generated, success message displayed
**Actual Result**:
**Pass/Fail**:

### TC-AM-007: Add Artist - Invalid Name
**Test Scenario**: Verify validation for invalid artist name
**Preconditions**: Artist Menu displayed
**Test Steps**:
1. Select "2" Artist Editor
2. Select "1" ADD Artist
3. Choose "Y" to add
4. Enter name with invalid characters (e.g., "@#$")
**Expected Result**: Validation error message displayed, re-prompt for name
**Actual Result**:
**Pass/Fail**:

### TC-AM-008: Add Artist - Invalid Phone
**Test Scenario**: Verify validation for invalid phone number
**Preconditions**: Artist Menu displayed
**Test Steps**:
1. Select "2" Artist Editor
2. Select "1" ADD Artist
3. Choose "Y" to add
4. Enter valid name and gender
5. Enter phone with letters (e.g., "abc123")
**Expected Result**: Validation error message displayed, re-prompt for phone
**Actual Result**:
**Pass/Fail**:

### TC-AM-009: Add Artist - Invalid Email
**Test Scenario**: Verify validation for invalid email
**Preconditions**: Artist Menu displayed
**Test Steps**:
1. Select "2" Artist Editor
2. Select "1" ADD Artist
3. Choose "Y" to add
4. Enter valid name, gender, phone
5. Enter email without @ (e.g., "test.com")
**Expected Result**: Validation error message displayed, re-prompt for email
**Actual Result**:
**Pass/Fail**:

### TC-AM-010: Edit Artist - Valid Edit
**Test Scenario**: Verify editing existing artist
**Preconditions**: Artist Menu displayed, artists exist
**Test Steps**:
1. Select "2" Artist Editor
2. Select "2" Edit Artist
3. Search and select artist
4. Enter new valid information
**Expected Result**: Artist information updated successfully
**Actual Result**:
**Pass/Fail**:

### TC-AM-011: Delete Artist - Confirm Delete
**Test Scenario**: Verify deleting artist with confirmation
**Preconditions**: Artist Menu displayed, artists exist
**Test Steps**:
1. Select "2" Artist Editor
2. Select "3" Delete Artist
3. Search and select artist
4. Confirm deletion with "Y"
**Expected Result**: Artist deleted, albums associated may be affected, success message
**Actual Result**:
**Pass/Fail**:

### TC-AM-012: Delete Artist - Cancel Delete
**Test Scenario**: Verify canceling artist deletion
**Preconditions**: Artist Menu displayed, artists exist
**Test Steps**:
1. Select "2" Artist Editor
2. Select "3" Delete Artist
3. Search and select artist
4. Cancel deletion with "N"
**Expected Result**: Artist not deleted, no changes made
**Actual Result**:
**Pass/Fail**:

### TC-AM-013: Export Artists to CSV
**Test Scenario**: Verify CSV export functionality
**Preconditions**: Artist Menu displayed, artists exist
**Test Steps**:
1. Select "3" Export Artists to CSV
**Expected Result**: "Artists exported to artists.csv" message, CSV file created
**Actual Result**:
**Pass/Fail**:

## Album Manager Tests

### TC-AL-001: Album Viewer - Display All (Empty)
**Test Scenario**: Verify display all albums when no albums exist
**Preconditions**: Album Menu displayed, no albums in system
**Test Steps**:
1. Select "1" Album Viewer
2. Select "1" Display All Albums
**Expected Result**: Message "Nothing to display. Please add an album." shown
**Actual Result**:
**Pass/Fail**:

### TC-AL-002: Album Viewer - Display All (With Data)
**Test Scenario**: Verify display all albums with existing data
**Preconditions**: Album Menu displayed, albums exist
**Test Steps**:
1. Select "1" Album Viewer
2. Select "1" Display All Albums
**Expected Result**: Table showing album details: No, Title, Artist ID, Album ID, Format, Date, Path
**Actual Result**:
**Pass/Fail**:

### TC-AL-003: Album Search by Artist
**Test Scenario**: Verify searching albums by artist
**Preconditions**: Album Menu displayed, albums exist
**Test Steps**:
1. Select "1" Album Viewer
2. Select "2" View Artist Albums By Search
3. Enter valid artist ID
**Expected Result**: Albums for that artist displayed
**Actual Result**:
**Pass/Fail**:

### TC-AL-004: Advanced Search - By Title
**Test Scenario**: Verify advanced search by album title
**Preconditions**: Album Menu displayed, albums exist
**Test Steps**:
1. Select "1" Album Viewer
2. Select "3" Advanced Search
3. Choose "1" Search by Album Title
4. Enter title prefix
**Expected Result**: Matching albums displayed
**Actual Result**:
**Pass/Fail**:

### TC-AL-005: Advanced Search - By Date Range
**Test Scenario**: Verify advanced search by date range
**Preconditions**: Album Menu displayed, albums exist
**Test Steps**:
1. Select "1" Album Viewer
2. Select "3" Advanced Search
3. Choose "2" Search by Date Range
4. Enter valid date range
**Expected Result**: Albums in date range displayed
**Actual Result**:
**Pass/Fail**:

### TC-AL-006: Add Album - Valid Data
**Test Scenario**: Verify adding album with valid information
**Preconditions**: Album Menu displayed, artists exist
**Test Steps**:
1. Select "2" Album Editor
2. Select "1" ADD Album
3. Choose "Y" to add
4. Select artist, enter valid album details
**Expected Result**: Album added successfully, ID generated
**Actual Result**:
**Pass/Fail**:

### TC-AL-007: Add Album - Invalid Date
**Test Scenario**: Verify validation for invalid date
**Preconditions**: Album Menu displayed, artists exist
**Test Steps**:
1. Select "2" Album Editor
2. Select "1" ADD Album
3. Choose "Y" to add
4. Select artist, enter title, format
5. Enter invalid date (e.g., "32/13/2023")
**Expected Result**: Validation error message, re-prompt for date
**Actual Result**:
**Pass/Fail**:

### TC-AL-008: Edit Album - Valid Edit
**Test Scenario**: Verify editing existing album
**Preconditions**: Album Menu displayed, albums exist
**Test Steps**:
1. Select "2" Album Editor
2. Select "2" Edit Album
3. Select artist and album
4. Enter new valid information
**Expected Result**: Album information updated successfully
**Actual Result**:
**Pass/Fail**:

### TC-AL-009: Delete Album - Single Album
**Test Scenario**: Verify deleting single album
**Preconditions**: Album Menu displayed, albums exist
**Test Steps**:
1. Select "2" Album Editor
2. Select "3" Delete Album
3. Select artist and specific album
4. Confirm deletion
**Expected Result**: Selected album deleted
**Actual Result**:
**Pass/Fail**:

### TC-AL-010: Delete Album - All Artist Albums
**Test Scenario**: Verify deleting all albums for an artist
**Preconditions**: Album Menu displayed, artist has multiple albums
**Test Steps**:
1. Select "2" Album Editor
2. Select "3" Delete Album
3. Select artist
4. Choose to delete all albums
**Expected Result**: All artist's albums deleted
**Actual Result**:
**Pass/Fail**:

### TC-AL-011: Export Albums to CSV
**Test Scenario**: Verify CSV export for albums
**Preconditions**: Album Menu displayed, albums exist
**Test Steps**:
1. Select "3" Export Albums to CSV
**Expected Result**: "Albums exported to albums.csv" message, CSV file created
**Actual Result**:
**Pass/Fail**:

## Error Handling Tests

### TC-EH-001: File Not Found on Startup
**Test Scenario**: Verify behavior when data files don't exist
**Preconditions**: No Artist.bin or Album.bin files
**Test Steps**:
1. Launch application
**Expected Result**: Files created automatically, application continues
**Actual Result**:
**Pass/Fail**:

### TC-EH-002: Invalid Menu Input - Non-numeric
**Test Scenario**: Verify handling of non-numeric menu input
**Preconditions**: Any menu displayed
**Test Steps**:
1. Enter letters instead of numbers (e.g., "abc")
**Expected Result**: Input cleared, error handling prevents crash, re-prompts
**Actual Result**:
**Pass/Fail**:

### TC-EH-003: Empty Search Input
**Test Scenario**: Verify handling of empty search strings
**Preconditions**: Search menu displayed
**Test Steps**:
1. Press Enter without typing anything
**Expected Result**: Appropriate handling (may show all or no results)
**Actual Result**:
**Pass/Fail**:

## Data Integrity Tests

### TC-DI-001: Artist ID Uniqueness
**Test Scenario**: Verify artist IDs are unique
**Preconditions**: Multiple artists added
**Test Steps**:
1. Add several artists
2. Check exported CSV or display
**Expected Result**: All artist IDs are unique
**Actual Result**:
**Pass/Fail**:

### TC-DI-002: Album ID Uniqueness
**Test Scenario**: Verify album IDs are unique
**Preconditions**: Multiple albums added
**Test Steps**:
1. Add several albums
2. Check exported CSV or display
**Expected Result**: All album IDs are unique
**Actual Result**:
**Pass/Fail**:

### TC-DI-003: Data Persistence
**Test Scenario**: Verify data persists between application runs
**Preconditions**: Data added in previous session
**Test Steps**:
1. Exit application
2. Restart application
3. View data
**Expected Result**: Previously added data still exists
**Actual Result**:
**Pass/Fail**:

## Performance Tests

### TC-PF-001: Large Dataset Display
**Test Scenario**: Verify performance with many records
**Preconditions**: Large number of artists/albums (if possible to create)
**Test Steps**:
1. Add many records
2. Display all
**Expected Result**: Displays within reasonable time, no crashes
**Actual Result**:
**Pass/Fail**:

### TC-PF-002: Search Performance
**Test Scenario**: Verify search works efficiently
**Preconditions**: Many records exist
**Test Steps**:
1. Perform searches on large dataset
**Expected Result**: Search results return quickly
**Actual Result**:
**Pass/Fail**:

## Boundary Tests

### TC-BT-001: Maximum Input Length - Name
**Test Scenario**: Verify handling of maximum name length
**Preconditions**: Add artist menu
**Test Steps**:
1. Enter name at maximum allowed length (49 chars)
**Expected Result**: Name accepted and stored correctly
**Actual Result**:
**Pass/Fail**:

### TC-BT-002: Maximum Input Length - Email
**Test Scenario**: Verify handling of long email
**Preconditions**: Add artist menu
**Test Steps**:
1. Enter very long email address
**Expected Result**: Email handled appropriately (truncated or validated)
**Actual Result**:
**Pass/Fail**:

### TC-BT-003: Date Boundary - Leap Year
**Test Scenario**: Verify leap year date handling
**Preconditions**: Add album menu
**Test Steps**:
1. Enter February 29, 2024 (leap year)
**Expected Result**: Date accepted
**Actual Result**:
**Pass/Fail**:

### TC-BT-004: Date Boundary - Non-Leap Year
**Test Scenario**: Verify non-leap year date rejection
**Preconditions**: Add album menu
**Test Steps**:
1. Enter February 29, 2023 (non-leap year)
**Expected Result**: Date rejected with error
**Actual Result**:
**Pass/Fail**:

## Test Summary
- **Total Test Cases**: 47
- **Passed**: [To be filled]
- **Failed**: [To be filled]
- **Not Executed**: [To be filled]

## Notes
- Update Actual Result and Pass/Fail columns during testing
- Document any bugs found with detailed reproduction steps
- Test on different environments if possible (Windows, Linux, macOS)
- Verify CSV export files contain correct data format</content>
<parameter name="filePath">c:\Users\Leul\Downloads\Telegram Desktop\Freshman\SEM 2\CS 222 Computer Programing 2\Lec\Assignment\GP Work\Album Management System\test_tasks.md
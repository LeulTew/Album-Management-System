@echo off
echo Testing Album Management System
echo ===============================

REM Clean up any existing data files
if exist Artist.bin del Artist.bin
if exist Album.bin del Album.bin

echo Building the application...
REM Assuming CodeBlocks or similar build system
REM For now, we'll assume the exe exists

if not exist album_management.exe (
    echo ERROR: album_management.exe not found. Please build the project first.
    pause
    exit /b 1
)

echo.
echo Running basic functionality test...
echo.

REM Test 1: Add an artist
echo Test 1: Adding an artist
echo 1 | album_management.exe > test_output1.txt 2>&1
echo 1 | album_management.exe >> test_output1.txt 2>&1
echo John Doe | album_management.exe >> test_output1.txt 2>&1
echo M | album_management.exe >> test_output1.txt 2>&1
echo 123456789 | album_management.exe >> test_output1.txt 2>&1
echo john@example.com | album_management.exe >> test_output1.txt 2>&1
echo 4 | album_management.exe >> test_output1.txt 2>&1
echo 5 | album_management.exe >> test_output1.txt 2>&1

echo.
echo Test 1 completed. Check test_output1.txt for results.

REM Test 2: View artists
echo.
echo Test 2: Viewing artists
echo 1 | album_management.exe > test_output2.txt 2>&1
echo 1 | album_management.exe >> test_output2.txt 2>&1
echo 1 | album_management.exe >> test_output2.txt 2>&1
echo 3 | album_management.exe >> test_output2.txt 2>&1
echo 4 | album_management.exe >> test_output2.txt 2>&1
echo 5 | album_management.exe >> test_output2.txt 2>&1

echo.
echo Test 2 completed. Check test_output2.txt for results.

REM Test 3: Statistics
echo.
echo Test 3: Checking statistics
echo 3 | album_management.exe > test_output3.txt 2>&1
echo 4 | album_management.exe >> test_output3.txt 2>&1
echo 5 | album_management.exe >> test_output3.txt 2>&1

echo.
echo Test 3 completed. Check test_output3.txt for results.

echo.
echo All tests completed!
echo Check the test_output*.txt files for detailed results.
echo.
pause
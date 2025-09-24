# Album Management System

A modernized C++ console application for managing music albums and artists, featuring advanced search, statistics, and data export capabilities.

## Features

- **Artist Management**: Add, edit, delete, and search artists
- **Album Management**: Add, edit, delete, and search albums
- **Advanced Search**: Search by title prefix or date range
- **Statistics**: View album counts per artist
- **CSV Export**: Export artists and albums to CSV files
- **Modern C++**: Uses std::string, std::vector, and other modern features

## Prerequisites

- C++11 compatible compiler (GCC, Clang, MSVC)
- CodeBlocks IDE (recommended for Windows)

## Building

### Using CodeBlocks
1. Open `Album Management System 1.0.cbp` in CodeBlocks
2. Click Build → Build
3. The executable will be created in the `bin/Debug/` or `bin/Release/` directory

### Using Command Line (GCC)
```bash
g++ -std=c++11 main.cpp manager.cpp -o album_management.exe
```

## Running

Execute the compiled binary:
```bash
./album_management.exe
```

## Testing

### Automated Tests
Run the PowerShell test suite:
```powershell
.\test_suite.ps1
```

Or use the batch file:
```cmd
run_tests.bat
```

### Manual Testing
1. **Add an Artist**:
   - Choose option 1 (Manage Artist)
   - Choose option 2 (Artist Editor)
   - Choose option 1 (Add Artist)
   - Enter artist details

2. **Add an Album**:
   - Choose option 2 (Manage Album)
   - Choose option 2 (Album Editor)
   - Choose option 1 (Add Album)
   - Select an artist and enter album details

3. **Test Export**:
   - Go to Artist Manager → Option 3 (Export to CSV)
   - Go to Album Manager → Option 3 (Export to CSV)
   - Check for `artists.csv` and `albums.csv` files

4. **Test Search**:
   - Go to Album Manager → Album Viewer → Advanced Search
   - Try title search or date range search

5. **View Statistics**:
   - Choose option 3 (Statistics) from main menu

## Project Structure

```
Album Management System/
├── main.cpp              # Main entry point
├── manager.h             # Header file with declarations
├── manager.cpp           # Implementation of all functions
├── tasks.md              # Project development tasks
├── .gitignore           # Git ignore file
├── test_suite.ps1       # Automated test suite
├── run_tests.bat        # Batch test runner
└── Group Members.txt    # Project team information
```

## Modernization Changes

This project has been modernized from legacy C++ to modern standards:

- **Memory Management**: Replaced raw pointers and C-strings with smart pointers and std::string
- **Containers**: Converted arrays to std::vector
- **File I/O**: Maintained binary file compatibility
- **Code Structure**: Improved function organization and error handling
- **Features**: Added export, advanced search, and statistics

## Version Control

The project uses Git for version control. All changes are committed with descriptive messages.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests
5. Submit a pull request

## License

This project is for educational purposes.
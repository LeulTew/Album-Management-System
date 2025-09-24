#include <iostream>
using namespace std;
#include <conio.h>
#include <fstream>
#include "manager.h"
#include "manager.cpp"

int main()
{
    fstream ArtFile;
    fstream AlbFile;
    artistList artist = {};
    albumList album = {};
    indexSet result = {};
    indexSet delArtArray={}, delArtFile={}, delAlbArray={}, delAlbFile={};

    welcome();
    if(!loading(ArtFile,AlbFile,artist,album,delArtFile,delAlbFile))
        return 0;
    mainH(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
    farewell();
    return 0;
}

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include "manager.h"

using namespace std;

void welcome()  //2
{
    system("COLOR 2F");
	cout<<"\n\n";
	cout<<setfill('~')<<setw(150)<<'~';
	cout<<"\n";
	cout<<"\n                          |o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o|     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |                    WELCOME                      |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |              o ALBUM MANAGEMENT o               |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |             Enter ENTER to continue...          |     ";
	cout<<"\n                          |                                                 |     ";
	cout<<"\n                          |o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o*o|     ";
	cout<<'\n'<<setw(150)<<'~'<<setfill(' ');
	cout<<endl<<endl;
    system("pause");
}

//
void printError(int errId){
    switch (errId){
        case 1:
            cout << "\t\n Error~ Artist file can not be opened!" << endl;
            break;
        case 2:
            cout << "\t\n Error~ Album file can not be opened!" << endl;
            break;
        case 3:
            cout << "\t\n Error~No sufficient memory. Program can not continue!" << endl;
            break;
        case 4:
            cout << "\t\n Error~no search results found.\n" << endl;
            break;
        case 5:
            cout << " \t\n Error~ No sufficient memory to create a space for the result array!" << endl;
            break;
    }
}

//
void intToCharArray(int last, char id[], char prefix[])
{
    int i=6;
    strcpy(id, prefix);
    while(i>2){
        id[i--] = last%10 + '0';
        last /= 10;
    }
}

//
int charArrayToInt(char *arr)
{
    int value, flag, r;
    flag = 1;
    value = 0;
    for( size_t i=3; i<strlen(arr); i++){
        if( i==0 && arr[i]=='-' ){
            flag = -1;
            continue;
        }
        r = arr[i] - '0';
        value = value * 10 + r;
    }
    value = value * flag;
    return value;
}

//
void openFile(std::fstream& fstr, const std::string& path)
{
    cout<<'\n';
    fstr.open(path, ios::in | ios::out | ios::binary);
    if(fstr) return;
    fstr.clear();
    ofstream ofs(path, ios::out | ios::binary);
    if(ofs){
        ofs.close();
        fstr.open(path, ios::in | ios::out | ios::binary);
        if(fstr) return;
    }
    throw FileException("Failed to open file: " + path);
}

std::string intToString(int last, const std::string& prefix) {
    return prefix + std::to_string(last);
}

int stringToInt(const std::string& arr) {
    // Assuming format is prefix + number, extract the number part
    size_t start = 0;
    while (start < arr.size() && !isdigit(arr[start])) start++;
    if (start == arr.size()) return 0;
    return std::stoi(arr.substr(start));
}

//3
bool loading(fstream & ArtFile, fstream & AlbFile, artistList & artist, albumList & album, indexSet & delArtFile, indexSet & delAlbFile)
{
    int i = 0;
 	char load[26];
 	while(i < 25)
 	{
 		system("cls");
 		load[i++] = '#';
 		load[i] = '\0';
        cout<<"\n\n\n\n\n\n\n\t\t\t\tLOADING: "<< load;
 		usleep(199900);
 	}
    system("clear");
    cout<<"\n";
    system("cls");

    // Check if files are empty and create sample data if needed
    if (!loadArtist(ArtFile, artist, delArtFile) || artist.artList.empty()) {
        // createSampleData(ArtFile, AlbFile, artist, album);
        // Reload after creating sample data
        // artist.artList.clear();
        // album.albList.clear();
        // loadArtist(ArtFile, artist, delArtFile);
    }

    if(!loadAlbum(AlbFile, album, delAlbFile))
        return false;

    return true;
}

//5
bool loadArtist(std::fstream& ArtFile, artistList& artist, indexSet& delArtFile)
{
    ArtistFile artFile;
    int nRec, pos;
    std::string id;

    try {
        openFile(ArtFile, artistFilePath);
    } catch(const FileException& e) {
        printError(1);
        system("pause");
        return false;
    }
    ArtFile.seekg(0, ios::end);
    nRec = ArtFile.tellg() / sizeof(artFile);
    artist.artList.reserve(nRec + DEFAULT_SIZE);
    ArtFile.seekg(0, ios::beg);
    pos = 0;
    for (int i = 0; i < nRec; i++){
        ArtFile.read((char*)&artFile, sizeof(artFile));
        if(std::string(artFile.artistIds) != "-1"){
            artist.artList.push_back({std::string(artFile.artistIds), std::string(artFile.names), pos});
            id = intToString(lastArtistID, "art");
            if(std::string(artFile.artistIds) > id){
                lastArtistID = stringToInt(std::string(artFile.artistIds));
            }
        }
        else{
            delArtFile.indexes.push_back(pos);
        }
        pos = ArtFile.tellg();
    }
    sortArtist(artist);
    return true;
}

//6
bool loadAlbum(std::fstream& AlbFile, albumList& album, indexSet& delAlbFile)
{
    AlbumFile albFile;
    int nRec, pos;

    try {
        openFile(AlbFile, albumFilePath);
    } catch(const FileException& e) {
        printError(2);
        system("pause");
        return false;
    }
    AlbFile.seekg(0, ios::end);
    nRec = AlbFile.tellg() / sizeof(albFile);
    album.albList.reserve(nRec + DEFAULT_SIZE);
    AlbFile.seekg(0, ios::beg);
    pos = 0;
    for (int i = 0; i < nRec; i++){
        AlbFile.read((char*)&albFile, sizeof(albFile));
        if (std::string(albFile.albumIds) != "-1"){
            album.albList.push_back(albumIndex{std::string(albFile.albumIds), std::string(albFile.artistIdRefs), std::string(albFile.titles), pos});
            std::string id = intToString(lastAlbumID, "alb");
            if (std::string(albFile.albumIds) > id){
                lastAlbumID = stringToInt(std::string(albFile.albumIds));
            }
        }
        else{
            delAlbFile.indexes.push_back(pos);
        }
        pos = AlbFile.tellg();
    }
    sortAlbum(album);
    return true;
}

//7
void sortArtist(artistList& artist)
{
    std::sort(artist.artList.begin(), artist.artList.end(), [](const artistIndex& a, const artistIndex& b) {
        return a.name < b.name;
    });
}

//8
void sortAlbum(albumList& album)
{
    std::sort(album.albList.begin(), album.albList.end(), [](const albumIndex& a, const albumIndex& b) {
        return a.artistId < b.artistId;
    });
}

//9
void mainH(fstream & ArtFile, fstream & AlbFile, artistList & artist, albumList & album, indexSet & result, indexSet & delArtArray, indexSet & delAlbArray)
{
    bool exit=false;
    do
    {
        int choice=mainMenu();
        if (choice ==1)
            exit=artistManager(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
        if (choice ==2)
            exit=albumManager(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
        if (choice ==3)
            displayStatistics(artist, album);
        if (choice ==4)
            exit=true;
    }while (!exit);
}

//10
int mainMenu()
{
    int c;
    do{
        system("COLOR 3F");
        system("cls");
        cout<<"\n";
        cout<<setfill('~')<<setw(80)<<'~';
        cout<<"\n\n";
        cout<<"                             ALBUM MANAGEMENT                              \n";
        cout<<"\n                                 *MENU*               ";
        cout<<"\n\n                       Enter  1 :  >> Manage Artist                           ";
        cout<<"\n\n                       Enter  2 :  >> Manage Album                            ";
        cout<<"\n\n                       Enter  3 :  >> Statistics                              ";
        cout<<"\n\n                       Enter  4 :  >> EXIT.                              \n\n ";
        cout<<setw(80)<<'~'<<setfill(' ');
        cout<<"\n Choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>4 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>4 || c<1);
    return c;
}

void displayStatistics(const artistList& artist, const albumList& album)
{
    system("cls");
    cout << "\n\n\t\t\tSTATISTICS\n\n";
    cout << "Total Artists: " << artist.artList.size() << endl;
    cout << "Total Albums: " << album.albList.size() << endl;
    cout << "\nAlbums per Artist:\n";
    for (const auto& art : artist.artList) {
        int count = 0;
        for (const auto& alb : album.albList) {
            if (alb.artistId == art.artistId) {
                count++;
            }
        }
        cout << art.name << ": " << count << " albums" << endl;
    }
    cout << endl << endl;
    system("pause");
}

void exportArtistsToCSV(const artistList& artist, const std::string& filename) {
    std::ofstream file(filename);
    if (!file) {
        std::cout << "Error opening file for export." << std::endl;
        return;
    }
    file << "ID,Name,Gender,Phone,Email\n";
    std::fstream ArtFile;
    try {
        openFile(ArtFile, artistFilePath);
    } catch(const FileException& e) {
        std::cout << "Error opening artist file." << std::endl;
        return;
    }
    ArtistFile artFile;
    for (const auto& art : artist.artList) {
        ArtFile.seekg(art.pos, ios::beg);
        ArtFile.read((char*)&artFile, sizeof(artFile));
        file << art.artistId << "," << art.name << "," << artFile.genders << "," << std::string(artFile.phones) << "," << std::string(artFile.emails) << "\n";
    }
    ArtFile.close();
    file.close();
    std::cout << "Artists exported to " << filename << std::endl;
}

void exportAlbumsToCSV(const albumList& album, const std::string& filename) {
    std::ofstream file(filename);
    if (!file) {
        std::cout << "Error opening file for export." << std::endl;
        return;
    }
    file << "AlbumID,ArtistID,Title,RecordFormat,DatePublished,Path\n";
    std::fstream AlbFile;
    try {
        openFile(AlbFile, albumFilePath);
    } catch(const FileException& e) {
        std::cout << "Error opening album file." << std::endl;
        return;
    }
    AlbumFile albFile;
    for (const auto& alb : album.albList) {
        AlbFile.seekg(alb.pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        file << alb.albumId << "," << alb.artistId << "," << std::string(albFile.titles) << "," << std::string(albFile.recordFormats) << "," << std::string(albFile.datePublished) << "," << std::string(albFile.paths) << "\n";
    }
    AlbFile.close();
    file.close();
    std::cout << "Albums exported to " << filename << std::endl;
}

//11
bool artistManager(fstream & ArtFile, fstream & AlbFile, artistList &artist, albumList &album, indexSet & result, indexSet & delArtArray, indexSet & delAlbArray)
{
    bool exit;
    do
    {
        exit=false;
        int choice=artistMenu();
        if (choice ==1)
            exit=artistViewer(ArtFile,artist,result);
        if (choice ==2)
            exit=artistEditor(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
        if (choice ==3) {
            exportArtistsToCSV(artist, "artists.csv");
            cout << endl << endl;
            system("pause");
            exit = true;
        }
        if (choice ==4)
            return false;
        if (choice ==5)
            return true;
    }while(exit);
    return true;
}

//12
int artistMenu()
{
    int c;
    do{
        system("COLOR 3E");
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *ArtistMenu*               ";
        cout<<"\n\n                       Enter  1 :  >> Artist Viewer                           ";
        cout<<"\n\n                       Enter  2 :  >> Artist Editor                            ";
        cout<<"\n\n                       Enter  3 :  >> Export Artists to CSV                              ";
        cout<<"\n\n                       Enter  4 :  >> Go To Main Menu                              ";
        cout<<"\n\n                       Enter  5 :  >> EXIT.                              \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>5 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>5 || c<1);
    return c;
}

//13
bool artistViewer(std::fstream& ArtFile, const artistList& artist, indexSet& result)
{
    bool exit=false;
    do
    {
        int choice=viewArtistMenu();
        if (choice ==1)
            displayAllArtist(ArtFile,artist);
        if (choice ==2)
            viewArtistBySearch(ArtFile,artist,result);
        if (choice ==3)
            exit=true;
    }while(!exit);
    return true;
}

//14
int viewArtistMenu()
{
    int c;
    system("COLOR 4E");
    do{
    system("cls");
    cout<<"\n\n";
    cout<<"\n                                 *View ArtistMenu*               ";
    cout<<"\n\n                       Enter  1 :  >> Display All Artist    ";
    cout<<"\n\n                       Enter  2 :  >> View Artist By Search  ";
    cout<<"\n\n                       Enter  3 :  >> GO BACK.          \n\n ";
    cout<<"\n choice:    ";
    cin>>c;
    cin.clear();
    cin.ignore(INT_MAX,'\n');
    if (c>3 || c<1){
        cout<<"Wrong Choice!";
        cout<<endl<<endl;
        system ("pause");
        system ("cls");
    }
    }while(c>3 || c<1);
    return c;
}

//15
void displayAllArtist(std::fstream& ArtFile, const artistList& artist)
{
    system("cls");
    ArtistFile artFile;
    int idx = 0;

    cout << "   " << left << setw(4) << "No" << setw(25) << "Name" << setw(12) << "Artist ID" << setw(8) << "Gender" << setw(15) << "Phone" << setw(30) << "Email" << endl;
    cout << "   " << string(90, '-') << endl;
    for(size_t i = 0; i < artist.artList.size(); ++i){
        ArtFile.seekg(artist.artList[i].pos, ios::beg);
        ArtFile.read((char*)&artFile, sizeof(artFile));
        cout << "   " << setw(4) << idx+1
             << setw(25) << std::string(artFile.names).c_str()
             << setw(12) << std::string(artFile.artistIds).c_str()
             << setw(8) << artFile.genders
             << setw(15) << std::string(artFile.phones).c_str()
             << setw(30) << std::string(artFile.emails).c_str() << endl;
        ++idx;
    }
    if(idx == 0){
        system("cls");
        cout << "\nThere is nothing to display.\n" ;
    }
    cout << endl << endl;
    system("pause");
}

//16
void viewArtistBySearch(std::fstream& ArtFile, const artistList& artist, indexSet& result)
{
    searchArtist(ArtFile,artist,result);
    displaySearchResult(ArtFile,artist,result);
    cout<<endl<<endl;
    system("pause");
}

//17
bool searchArtist(std::fstream& ArtFile, const artistList& artist, indexSet& result)
{
    std::string targetId, targetName;
    int searchby;
    cout << "\t1. Search artist by ID \n\t2. Search artist by Name" << endl;
    do{
        cout << "\t  Choice: ";
        cin >> searchby;
        cin.ignore(INT_MAX, '\n');
        switch (searchby){
        case 1:
            cout << "\nEnter prefix of Id of Artist: ";
            getline(cin, targetId);
            if (!searchArtistById(artist, result, targetId))
                return false;
            break;
        case 2:
            cout << "\nEnter prefix of name of Artist: ";
            getline(cin, targetName);
            if (!searchArtistByName(artist, result, targetName))
                return false;
            break;
        default:
            cout << "Wrong choice. Enter again.\n" << endl;
            break;
        }
    }while(searchby != 1 && searchby != 2);

    return true;
}

//18
bool searchArtistById(const artistList& artist, indexSet& result, const std::string& targetId)
{
    result.indexes.clear();
    for(size_t i = 0; i < artist.artList.size(); i++){
        if(artist.artList[i].artistId.find(targetId) == 0){
            result.indexes.push_back(i);
        }
    }
    return !result.indexes.empty();
}

//19
bool searchArtistByName(const artistList& artist, indexSet& result, const std::string& targetName)
{
    result.indexes.clear();
    for(size_t i = 0; i < artist.artList.size(); i++){
        if(artist.artList[i].name.find(targetName) == 0){
            result.indexes.push_back(i);
        }
    }
    return !result.indexes.empty();
}

//20
void displaySearchResult(std::fstream& ArtFile, const artistList& artist, const indexSet& result)
{
    ArtistFile artFile;
    if(result.indexes.empty()){
        printError(4);
        return;
    }else{
        cout << " \tArtist Search Results:" << endl;
        cout << "\t" << result.indexes.size() << " artist found." << endl << endl;
        cout << "I\t" << "Ids\t" << setw(10) << "\tNames\t" << setw(29) << "Gender \t" << "Phone" << setw(15) << "\tEmail" << endl;
        for (size_t i = 0; i < result.indexes.size(); i++)
        {
            size_t idx = result.indexes[i];
            ArtFile.seekg(artist.artList[idx].pos, ios::beg);
            ArtFile.read((char*)&artFile, sizeof(ArtistFile));
            cout << i+1 << '\t' << std::string(artFile.artistIds) << setw(8) << '\t' << std::string(artFile.names) << setw(30 - std::string(artFile.names).length()) << artFile.genders << '\t' << setw(18) << std::string(artFile.phones) << '\t' << std::string(artFile.emails) << endl;
        }

    }
    // result.indexes.clear(); not needed since const
}

//21
bool artistEditor(fstream & ArtFile, fstream & AlbFile, artistList &artist, albumList &album, indexSet & result, indexSet & delArtArray, indexSet & delAlbArray)
{
    bool exit=false, success=false;
    do
    {
        int choice=editArtistMenu();
        if (choice ==1){
            success= addArtist(ArtFile,artist);
            if(success)
                cout<<"Artist Added Successfully! "<<endl;
            else
                cout<<"Artist not added. "<<endl;
        cout<<endl<<endl;
        system("pause");
        }
        if (choice ==2)
            editArtist(ArtFile,artist,result);
        if (choice ==3)
            deleteArtist(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
        if (choice ==4)
            exit=true;
    }while(!exit);
    return true;
}

//22
int editArtistMenu()
{
    int c;
    system("COLOR 2E");
    do{
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Edit Artist Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> ADD Artist    ";
        cout<<"\n\n                       Enter  2 :  >> Edit Artist  ";
        cout<<"\n\n                       Enter  3 :  >> Delete Artist  ";
        cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>4 || c<1){
            cout<<"Wrong Choice!"<<endl;
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
            }
    }while(c>4 || c<1);
    return c;
}

//23
bool addArtist(std::fstream& ArtFile, artistList& artist)
{
    char addA;
    system("cls");
    cout << "Do you want to add an artist? (Y/N) : ";
    cin >> addA;
    if (addA == 'y' || addA == 'Y')
    {
        int pos;
        Artist art = getArtistInfo();
        std::string id = intToString(++lastArtistID, "art");
        art.setArtistId(id);
        ArtistFile artFile;
        strncpy(artFile.artistIds, art.getArtistId().c_str(), 7);
        artFile.artistIds[7] = '\0';
        strncpy(artFile.names, art.getName().c_str(), 49);
        artFile.names[49] = '\0';
        artFile.genders = art.getGender();
        strncpy(artFile.phones, art.getPhone().c_str(), 14);
        artFile.phones[14] = '\0';
        strncpy(artFile.emails, art.getEmail().c_str(), 49);
        artFile.emails[49] = '\0';
        ArtFile.seekp(0, ios::end);
        pos = ArtFile.tellp();
        ArtFile.write((char*)&artFile, sizeof(ArtistFile));
        artist.artList.push_back({art.getArtistId(), art.getName(), pos});
        sortArtist(artist);
        return true;
    }else
        return false;
}

//24
Artist getArtistInfo()
{
    Artist art;
    cin.ignore();
    art.setName(getArtistName());
    art.setGender(getArtistGender());
    cin.ignore();
    art.setPhone(getArtistPhone());
    art.setEmail(getArtistEmail());
    return art;
}

//25
std::string getArtistName()
{
    std::string name;
    while(true)
    {
        cout << "Enter Artist name: ";
        getline(cin, name);
        try {
            validateName(name);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }
    name = formatName(name);
    return name;
}

//26
char getArtistGender()
{
    char gender;
    do
    {
        cout<<"Enter Artist Gender (M/F): ";
        cin>>gender;
        if (gender>='a' && gender<='z'){
                gender-=32;}
        try {
            validateGender(gender);
            return gender;
        } catch(const ValidationException& e) {
            cout<<e.what()<<endl;
        }
    }while(true);
}

//27
std::string getArtistPhone()
{
    std::string phone;
    do
    {
        cout << "Enter Artist Phone Number: ";
        getline(cin, phone);
        cin.clear();
        cin.ignore(INT_MAX, '\n');
        try {
            validatePhone(phone);
            return phone;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
}

//28
std::string getArtistEmail()
{
    std::string email;
    do
    {
        cout << "<sample@email.com> or <sample@email> \nEnter Artist email: ";
        getline(cin, email);
        try {
            validateEmail(email);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    email = formatEmail(email);
    return email;
}

//29.	validateName
void validateName(const std::string& name)
{
    if (name.empty()){
        throw ValidationException("Artist name cannot be empty!");
    }else if (name[0] == ' '){
        throw ValidationException("Artist name cannot start with a space!");
    }else{
        for(char c : name){
            if(!(c == ' ' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))){
                throw ValidationException("Artist name contains invalid characters!");
            }
        }
    }
}

//30
std::string formatName(std::string name)
{
    for (size_t i = 0; i < name.length(); i++)
    {
        if(i == 0 || name[i-1] == ' '){
            if (name[i] >= 'a' && name[i] <= 'z'){
                name[i] -= 32;}
        }else{
            if(name[i] >= 'A' && name[i] <= 'Z'){
                name[i] += 32;}
        }
    }
    return name;
}

//31
void validateGender(char gender)
{
    if (!(gender =='M' || gender =='F'))
        throw ValidationException("Artist gender should be male(M) or female(F)!");
}

//32
void validatePhone(const std::string& phone)
{
    if (phone.length() == 0)
        throw ValidationException("Phone number cannot be empty!");
    else{
        for(char c : phone){
            if(c < '0' || c > '9'){
                throw ValidationException("Phone number must contain only digits!");
            }
        }
    }
}

//33
void validateEmail(const std::string& email)
{
    int domain = 0;
    if (email.empty()){
        throw ValidationException("Email cannot be empty!");
    }else if (email[0] == ' ' || email[0] == '@'){
        throw ValidationException("Email cannot start with space or @!");
    }else{
        for(char c : email){
            if (c == '@')
                domain++;
        }
        if(domain != 1)
            throw ValidationException("Email must contain exactly one @!");
        }
}

//34
std::string formatEmail(std::string email)
{
    size_t pos = email.find(".com");
    if (pos != std::string::npos){
        // already has .com
    } else {
        email += ".com";
    }
    for(char& c : email){
        if (c >= 'A' && c <= 'Z')
            c += 32;
    }
    return email;
}

//35
void editArtist(std::fstream& ArtFile, artistList& artist, indexSet& result)
{
    system("cls");
    cout << setw(30) << "Edit Artist " << endl;
    do{
        searchArtist(ArtFile, artist, result);
        if (result.indexes.empty()){
            printError(4);
            system("pause");
            return;
        }
    }while(result.indexes.empty());
    size_t idx = selectArtist(ArtFile, artist, result, "edit");
    editArtistInfo(ArtFile, artist, idx);
    sortArtist(artist);
}

//36
int selectArtist(std::fstream& ArtFile, const artistList& artist, indexSet& result, const std::string& forWhat)
{
    int s;
    cout << result.indexes.size() << " results found.\n";
    system("pause");
    if (result.indexes.empty())
        return 0;
    for(size_t i = 0; i < result.indexes.size(); i++)
        cout << '\t' << i+1 << ". " << artist.artList[result.indexes[i]].name << endl;
    cout << "\n\t Select Artist to " << forWhat << ':';
    do{
        cin >> s;
        if( s < 1 || s > (int)result.indexes.size() )
             cout << "Wrong choice. Try Again." << endl;
    }while( s < 1 || s > (int)result.indexes.size() );
    return result.indexes[s-1];
}

//37
bool editArtistInfo(std::fstream& ArtFile, artistList& artist, int idx)
{
    int pos;
    displayOneArtist(ArtFile, artist, idx);
    Artist art = getArtistInfo();
    art.setArtistId(artist.artList[idx].artistId);
    ArtistFile artFile;
    strncpy(artFile.artistIds, art.getArtistId().c_str(), 7);
    artFile.artistIds[7] = '\0';
    strncpy(artFile.names, art.getName().c_str(), 49);
    artFile.names[49] = '\0';
    artFile.genders = art.getGender();
    strncpy(artFile.phones, art.getPhone().c_str(), 14);
    artFile.phones[14] = '\0';
    strncpy(artFile.emails, art.getEmail().c_str(), 49);
    artFile.emails[49] = '\0';
    ArtFile.seekp(artist.artList[idx].pos, ios::beg);
    pos = ArtFile.tellp();
    ArtFile.write((char*)&artFile, sizeof(ArtistFile));
    artist.artList[idx].artistId = art.getArtistId();
    artist.artList[idx].name = art.getName();
    artist.artList[idx].pos = pos;
    cout << "\n\tEdited \n\n";
    system("pause");
    return true;
}

//38
void displayOneArtist(std::fstream& ArtFile, const artistList& artist, int idx)
{
    ArtistFile artFile;
    ArtFile.seekg(artist.artList[idx].pos, ios::beg);
    ArtFile.read((char*)&artFile, sizeof(artFile));
    cout << endl << endl;
    cout << "\tId:     " << std::string(artFile.artistIds) << endl;
    cout << "\tName:   " << std::string(artFile.names) << endl;
    cout << "\tGender: " << artFile.genders << endl;
    cout << "\tPhone:  " << std::string(artFile.phones) << endl;
    cout << "\tEmail:  " << std::string(artFile.emails) << endl;
    cout << endl << endl;
    system("pause");
}

//39
void deleteArtist(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray)
{
    system("cls");
    cout << setw(30) << "Delete Artist " << endl;
    size_t selectedIdx;
    while(result.indexes.empty()){
        searchArtist(ArtFile, artist, result);
        selectedIdx = selectArtist(ArtFile, artist, result, "delete");
    }
    displayOneArtist(ArtFile, artist, selectedIdx);
    removeArtist(ArtFile, AlbFile, artist, album, delArtArray, delAlbArray, selectedIdx);
}

//40
void removeArtist(std::fstream& ArtFile, std::fstream& AlbFile, artistList& artist, albumList& album, indexSet& delArtArray, indexSet& delAlbArray, int idx)
{
    char remv;
    int pos;
    ArtistFile BLANK_ARTIST_FILE = {"-1", "", 'N', "", ""};
    do{
        cout << "Are you sure you want to remove the selected artist? (Y/N) : ";
        cin >> remv;
        if (remv == 'y' || remv == 'Y')
        {
            for (size_t i = 0; i < album.albList.size(); i++)
                if (artist.artList[idx].artistId == album.albList[i].artistId)
                    removeArtistAllAlbums(ArtFile, AlbFile, artist, album, delAlbArray, i);
            ArtFile.seekp(artist.artList[idx].pos, ios::beg);
            pos = ArtFile.tellp();
            ArtFile.write((char*)&BLANK_ARTIST_FILE, sizeof(ArtistFile));
            artist.artList[idx].artistId = BLANK_ARTIST_FILE.artistIds;
            artist.artList[idx].name = BLANK_ARTIST_FILE.names;
            artist.artList[idx].pos = pos;
            delArtArray.indexes.push_back(idx);
            cout << "\n\t Artist removed successfully! \n" << endl;
            system("pause");
        }else if(remv == 'n' || remv == 'N')
        {
            cout << "Artist not removed. \n" << endl;
            system("pause");
            return;
        }else{cout << "Wrong entry. Try again!" << endl;}
    }while(remv != 'y' && remv != 'Y' && remv != 'n' && remv != 'N');
}

//41
void removeArtistAllAlbums(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& delAlbArray, int i)
{
    int pos;
    AlbumFile BLANK_ALBUM_FILE = {"-1", "-1", "", "", "", ""};
    AlbFile.seekp(album.albList[i].pos, ios::beg);
    pos = AlbFile.tellp();
    AlbFile.write((char*)&BLANK_ALBUM_FILE, sizeof(AlbumFile));
    album.albList[i].albumId = BLANK_ALBUM_FILE.albumIds;
    album.albList[i].artistId = BLANK_ALBUM_FILE.artistIdRefs;
    album.albList[i].title = BLANK_ALBUM_FILE.titles;
    album.albList[i].pos = pos;
    delAlbArray.indexes.push_back(i);
}

//42
bool albumManager(fstream & ArtFile, fstream & AlbFile, artistList &artist, albumList &album, indexSet & result, indexSet & delArtArray, indexSet & delAlbArray)
{
    bool exit;
    do
    {
        exit=false;
        int choice=albumMenu();
        if (choice ==1)
            exit=albumViewer(AlbFile,album,result);
        if (choice ==2)
            exit=albumEditor(ArtFile,AlbFile,artist,album,result,delArtArray,delAlbArray);
        if (choice ==3) {
            exportAlbumsToCSV(album, "albums.csv");
            cout << endl << endl;
            system("pause");
            exit = true;
        }
        if (choice ==4)
            return false;
        if (choice ==5)
            return true;
    }while(exit);
    return true;
}

//43
int albumMenu()
{
    int c;
    do{
        system("COLOR 1B");
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Album Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> Album Viewer                           ";
        cout<<"\n\n                       Enter  2 :  >> Album Editor                            ";
        cout<<"\n\n                       Enter  3 :  >> Export Albums to CSV                              ";
        cout<<"\n\n                       Enter  4 :  >> Go To Main Menu                              ";
        cout<<"\n\n                       Enter  5 :  >> EXIT.                              \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>5 || c<1){
            cout<<"Wrong Choice!";
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
        }
    }while(c>5 || c<1);
    return c;
}

//44
bool albumViewer(std::fstream& AlbFile, const albumList& album, indexSet& result)
{
    bool exit=false;
    do
    {
        int choice=viewAlbumMenu();
        if (choice ==1)
            displayAllAlbums(AlbFile,album);
        if ( choice == 2 ){
            system("cls");
            std::string target;
            cout << "\nEnter prefix of Id of Artist: ";
            cin >> target;
            if(!searchAlbumByArtistId(AlbFile, album, result, target)){
                printError(4);
                system("pause");
            }
            else{
                displayAlbumSearchResult(AlbFile, album, result);
                cout << endl << endl;
                system("pause");
            }
        }
        if (choice ==3)
            advancedSearchAlbums(AlbFile, album, result);
        if (choice ==4)
            exit=true;
    }while(!exit);
    return true;
}

//45
int viewAlbumMenu()
{
    int c;
    system("COLOR 2E");
    do{
    system("cls");
    cout<<"\n\n";
    cout<<"\n                                 *View Album Menu*               ";
    cout<<"\n\n                       Enter  1 :  >> Display All Albums    ";
    cout<<"\n\n                       Enter  2 :  >> View Artist Albums By Search   ";
    cout<<"\n\n                       Enter  3 :  >> Advanced Search   ";
    cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
    cout<<"\n choice:    ";
    cin>>c;
    cin.clear();
    cin.ignore(INT_MAX,'\n');
    if (c>4 || c<1){
        cout<<"Wrong Choice!";
        cout<<endl<<endl;
        system ("pause");
        system ("cls");
    }
    }while(c>4 || c<1);
    return c;
}

//46
void displayAllAlbums(std::fstream& AlbFile, const albumList& album)
{
    system("cls");
    AlbumFile albFile;
    int idx = 0;
    cout << endl << "\tNo.\t" << "Titles\t" << setw(33) << "IdsRef" << setw(11) << "\tAlbumIds";
    cout << "\tRecordFormat \t" << "DatePublisheds" << setw(5) << "\tPaths" << endl;
    for (size_t i = 0; i < album.albList.size(); i++){
        if(album.albList[i].albumId != "-1"){
            AlbFile.seekg(album.albList[i].pos, ios::beg);
            AlbFile.read((char*)&albFile, sizeof(albFile));
            std::string title(albFile.titles);
            cout << '\t' << ++idx << '\t' << title << ' ' << setw(40 - title.length()) << album.albList[i].artistId;
            cout << '\t' << album.albList[i].albumId << setw(3) << '\t' << '.' << std::string(albFile.recordFormats) << setw(10) << '\t' << std::string(albFile.datePublished) << setw(5) << '\t' << std::string(albFile.paths) << endl;
        }
    }
    cout << endl << endl;
    if(idx == 0)
        cout << "\tNothing to display. Please add an album." << endl;
    system("pause");
}

//47
bool searchAlbumByArtistId(std::fstream& AlbFile, const albumList& album, indexSet& result, const std::string& targetId)
{
    result.indexes.clear();
    for(size_t i = 0; i < album.albList.size(); i++){
        if(album.albList[i].artistId != "-1"){
            if(album.albList[i].artistId.find(targetId) == 0){
               result.indexes.push_back(i);
            }
        }
    }
    return !result.indexes.empty();
}

//48
void displayAlbumSearchResult(std::fstream& AlbFile, const albumList& album, const indexSet& result)
{
    cout << endl << " \tAlbum Search Results:" << endl;
    AlbumFile albFile;
    cout << "\t" << result.indexes.size() << " Albums found." << endl << endl;
    cout << endl << "\tNo.\t" << "Titles" << setw(33) << "IdsRef" << setw(11) << "\tAlbumIds";
    cout << "\tRecordFormat \t" << "DatePublisheds" << setw(5) << "\tPaths" << endl;
    for (size_t i = 0; i < result.indexes.size(); i++){
        size_t idx = result.indexes[i];
        AlbFile.seekg(album.albList[idx].pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        std::string title(albFile.titles);
        cout << '\t' << idx << '\t' << title << setw(40 - title.length()) << album.albList[idx].artistId;
        cout << '\t' << album.albList[idx].albumId << setw(3) << '\t' << '.' << std::string(albFile.recordFormats) << setw(10) << '\t' << std::string(albFile.datePublished) << setw(5) << '\t' << std::string(albFile.paths) << endl;
    }
    cout << endl << endl;
}

//49
bool albumEditor(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result, indexSet& delArtArray, indexSet& delAlbArray)
{
    bool exit=false, success=false;
    do
    {
        int choice=editAlbumMenu();
        if (choice ==1){
            success=addAlbum(ArtFile,AlbFile,artist,album,result);
            if(success)
                cout<<"Artist Added Successfully! "<<endl;
            else
                cout<<"Artist not added. "<<endl;
        cout<<endl<<endl;
        system("pause");
        }
        if (choice ==2)
            editAlbum(ArtFile,AlbFile,artist,album,result);
        if (choice ==3)
            deleteAlbum(ArtFile,AlbFile,artist,album,result,delAlbArray);
        if (choice ==4)
            exit=true;
    }while(!exit);
    return true;
}

//50
int editAlbumMenu()
{
    int c;
    system("COLOR 2E");
    do{
        system("cls");
        cout<<"\n\n";
        cout<<"\n                                 *Edit Album Menu*               ";
        cout<<"\n\n                       Enter  1 :  >> ADD Album    ";
        cout<<"\n\n                       Enter  2 :  >> Edit Album  ";
        cout<<"\n\n                       Enter  3 :  >> Delete Album  ";
        cout<<"\n\n                       Enter  4 :  >> GO BACK.          \n\n ";
        cout<<"\n choice:    ";
        cin>>c;
        cin.clear();
        cin.ignore(INT_MAX,'\n');
        if (c>4 || c<1){
            cout<<"Wrong Choice!"<<endl;
            cout<<endl<<endl;
            system ("pause");
            system ("cls");
            }
    }while(c>4 || c<1);
    return c;
}

//51
bool addAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result)
{
    char addA;
    do{
        system("cls");
        cout << setw(30) << "Add Album " << endl;
        cout << "Do you want to add an album? (Y/N) : ";
        cin >> addA;
        if (addA == 'y' || addA == 'Y')
        {
            int pos, select;
            AlbumFile albFile;
            while(result.indexes.empty()){
                searchArtist(ArtFile, artist, result);
                if (result.indexes.empty()){
                    printError(4);
                    system("pause");
                }
            }
            select = selectArtist(ArtFile, artist, result, "add an album");
            albFile = getAlbumInfo();
            std::string idStr = intToString(++lastAlbumID, "alb");
            strncpy(albFile.albumIds, idStr.c_str(), 7);
            albFile.albumIds[7] = '\0';
            strncpy(albFile.artistIdRefs, artist.artList[select].artistId.c_str(), 7);
            albFile.artistIdRefs[7] = '\0';
            AlbFile.seekp(0, ios::end);
            pos = AlbFile.tellp();
            AlbFile.write((char*)&albFile, sizeof(albFile));
            album.albList.push_back(albumIndex{std::string(albFile.albumIds), std::string(albFile.artistIdRefs), std::string(albFile.titles), pos});
            sortAlbum(album);
            cout << endl;
            cout << " Album ID: " << albFile.albumIds << endl;
            cout << endl << endl;
            result.indexes.clear();
            return true;
        }else if (addA == 'n' || addA == 'N')
            return false;
        else{
            cout << "Wrong entry. Try again!" << endl;
            return false;
        }
    }while(addA != 'y' && addA != 'Y' && addA != 'n' && addA != 'N');
}

//52
AlbumFile getAlbumInfo()
{
    AlbumFile albFile;
    std::string title = getAlbumTitle();
    std::string recordFormat = getAlbumRecordFormat();
    std::string date = getAlbumDate();
    std::string path = getAlbumPath();
    strncpy(albFile.titles, title.c_str(), 79);
    albFile.titles[79] = '\0';
    strcpy(albFile.recordFormats, recordFormat.c_str());
    strncpy(albFile.datePublished, date.c_str(), 10);
    albFile.datePublished[10] = '\0';
    strcpy(albFile.paths, path.c_str());
    return albFile;
}

//53
std::string getAlbumTitle()
{
    std::string title;
    do
    {
        cout << "Enter album title: ";
        getline(cin, title);
        try {
            validateAlbumTitle(title);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    title = formatAlbumTitle(title);
    return title;
}

//54
std::string getAlbumRecordFormat()
{
    std::string albumFormat;
    do{
        cout << "Enter the record format of the album: ";
        getline(cin, albumFormat);
        try {
            validateAlbumFormat(albumFormat);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    albumFormat = formatAlbumFormat(albumFormat);
    return albumFormat;
}

//55
std::string getAlbumDate()
{
    unsigned int day, month, year;
    do{
        cout << "Enter the date published (DD/MM/YYYY): ";
        cin >> day;
        cin.ignore();
        cin >> month;
        cin.ignore();
        cin >> year;
        try {
            validateAlbumDate(day, month, year);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    return formatAlbumDate(day, month, year);
}

//56
std::string getAlbumPath()
{
    std::string albumPath;
    do{
        cout << "Enter album path: ";
        getline(cin, albumPath);
        try {
            validateAlbumPath(albumPath);
            break;
        } catch(const ValidationException& e) {
            cout << e.what() << endl;
        }
    }while(true);
    albumPath = formatAlbumPath(albumPath);
    return albumPath;
}

//57
void validateAlbumTitle(const std::string& albumTitle)
{
    if (albumTitle.empty()){
        throw ValidationException("Album title cannot be empty!");
    }else if (albumTitle[0] == ' '){
        throw ValidationException("Album title cannot start with a space!");
    }else{
        for(char c : albumTitle){
            if(!(c == ' ' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))){
                throw ValidationException("Album title contains invalid characters!");
            }
        }
    }
}

//58
std::string formatAlbumTitle(std::string albumTitle)
{
    for (size_t i = 0; i < albumTitle.length(); i++)
    {
        if(i == 0 || albumTitle[i-1] == ' '){
            if (albumTitle[i] >= 'a' && albumTitle[i] <= 'z'){
                albumTitle[i] -= 32;
            }
        }else{
            if(albumTitle[i] >= 'A' && albumTitle[i] <= 'Z'){
                albumTitle[i] += 32;
            }
        }
    }
    return albumTitle;
}

//59
void validateAlbumFormat(const std::string& albumFormat)
{
    std::string lowerFormat = albumFormat;
    for(char& c : lowerFormat) c = tolower(c);
    std::vector<std::string> validFormats = {"m4a", "flac", "mp3", "mp4", "wav", "wma", "aac", "dsd", "alac", "aiff"};
    for(const auto& fmt : validFormats){
        if(lowerFormat == fmt) return;
    }
    throw ValidationException("Invalid album record format!");
}

//60
std::string formatAlbumFormat(std::string albumFormat)
{
    for (char& c : albumFormat){
        if (c >= 'A' && c <= 'Z')
            c += 32;
    }
    return albumFormat;
}

//61
void validateAlbumDate(unsigned int day, unsigned int month, unsigned int year)
{
    if (month>12 ||day<1 || month<1 || year<0 )
        throw ValidationException("Invalid date: month/day/year out of range!");
    if(month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12){
        if (day>31)
            throw ValidationException("Invalid date: day exceeds 31 for the month!");
    }else if (day>30)
        throw ValidationException("Invalid date: day exceeds 30 for the month!");
    if(month==2)
    {
        if (year%4==0){
            if((year%100==0) && (year%400!=0)){  //Leap Year
                if(day>28)
                    throw ValidationException("Invalid date: February has only 28 days in this year!");
            }else if (day>29)
                throw ValidationException("Invalid date: February has only 29 days in leap year!");
        }else if (day>28)
                throw ValidationException("Invalid date: February has only 28 days!");
    }
}

//62
std::string formatAlbumDate(unsigned int day, unsigned int month, unsigned int year)
{
    char tempD[3], tempM[3], tempY[5];
    sprintf(tempD, "%02d", day);
    sprintf(tempM, "%02d", month);
    sprintf(tempY, "%04d", year);
    std::string date = std::string(tempD) + "/" + std::string(tempM) + "/" + std::string(tempY);
    return date;
}

//63
void validateAlbumPath(const std::string& albumPath)
{
    int slash = 0;
    if (albumPath.empty()){
        throw ValidationException("Album path cannot be empty!");
    }else if (albumPath[0] == ' '){
        throw ValidationException("Album path cannot start with a space!");
    }else{
        for(char c : albumPath){
            if (c == '\\')
                slash++;
        }
        if(slash < 1)
            throw ValidationException("Album path must contain at least one backslash!");
    }
}

//64
std::string formatAlbumPath(std::string albumPath)
{
    for (size_t i = 0; i < albumPath.length(); i++)
    {
        if(i == 0 || albumPath[i-1] == '\\'){
            if (albumPath[i] >= 'a' && albumPath[i] <= 'z'){
                albumPath[i] -= 32;
            }
        }else{
            if(albumPath[i] >= 'A' && albumPath[i] <= 'Z'){
                albumPath[i] += 32;
            }
        }
    }
    return albumPath;
}

//65
void editAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result)
{
    system("cls");
    cout << setw(30) << "Edit Album " << endl;
    int select;
    bool finish = false;
   while(result.indexes.empty()){
        searchArtist(ArtFile, artist, result);
        select = selectArtist(ArtFile, artist, result, "edit");
   }
    select = selectAlbum(AlbFile, artist, album, result, select, "edit");
    if(select == -1 )
        return;
    while (finish == false && !result.indexes.empty())
        finish = editAlbumInfo(AlbFile, album, select);
    sortAlbum(album);
}

//66
int selectAlbum(std::fstream& AlbFile, const artistList& artist, const albumList& album, indexSet& result, int idx, const std::string& forWhat)
{
    int s;
    result.indexes.clear();
    for(size_t i = 0; i < album.albList.size(); i++){
        if(album.albList[i].artistId != "-1"){
            if(artist.artList[idx].artistId == album.albList[i].artistId){
               result.indexes.push_back(i);
            }
        }
    }
    if(result.indexes.size() > 0)
        cout << endl << "   " << result.indexes.size() << " albums have been found." << endl << endl;
    else {
        cout << endl << "   The artist has no album. Please add an album first." << endl << endl;
        system("pause");
        return -1;
    }
    system("pause");
    if (result.indexes.size() == 1){
        displayOneAlbum(AlbFile, album, result.indexes[0]);
        return result.indexes[0];
    }
    else if (result.indexes.size() > 1){
        cout << "   Choose an album to " << forWhat << endl;
        for(size_t i = 0; i < result.indexes.size(); i++){
            cout << "       " << i+1 << ". " << album.albList[result.indexes[i]].title << endl;
        }
        cout << endl;
        do{
            cin >> s;
            if( s < 1 || s > (int)result.indexes.size() ){
                cout << "\tError~Choice must be between 1 and " << result.indexes.size() << ".Re-enter." << endl;
                system("pause");
            }
        }while( s < 1 || s > (int)result.indexes.size() );
        return result.indexes[s-1];
    }
    return -1;
}

//67
bool editAlbumInfo(std::fstream& AlbFile, albumList& album, int idx)
{
    AlbumFile albFile;
    bool edited = true;
    int pos;
    albFile = getAlbumInfo();
    strncpy(albFile.albumIds, album.albList[idx].albumId.c_str(), 7);
    albFile.albumIds[7] = '\0';
    strncpy(albFile.artistIdRefs, album.albList[idx].artistId.c_str(), 7);
    albFile.artistIdRefs[7] = '\0';
    AlbFile.seekp(album.albList[idx].pos, ios::beg);
    pos = AlbFile.tellp();
    AlbFile.write((char*)&albFile, sizeof(albFile));
    album.albList[idx].albumId = std::string(albFile.albumIds);
    album.albList[idx].artistId = std::string(albFile.artistIdRefs);
    album.albList[idx].title = std::string(albFile.titles);
    album.albList[idx].pos = pos;
    cout << "\n\tEdited\n\n";
    system("pause");
    return edited;
}

//68
void displayOneAlbum(std::fstream& AlbFile, const albumList& album, int idx)
{
    AlbumFile albFile;
    AlbFile.seekg(album.albList[idx].pos, ios::beg);
    AlbFile.read((char*)&albFile, sizeof(albFile));
    cout << endl << endl;
    cout << "\t\tTitle:          " << std::string(albFile.titles) << endl;
    cout << "\t\tAlbum ID:       " << std::string(albFile.albumIds) << endl;
    cout << "\t\tRecord Format:  ." << std::string(albFile.recordFormats) << endl;
    cout << "\t\tDate Published: " << std::string(albFile.datePublished) << endl;
    cout << "\t\tPath:           " << std::string(albFile.paths) << endl;
    cout << endl << endl;
    system("pause");
}

//69
void deleteAlbum(std::fstream& ArtFile, std::fstream& AlbFile, const artistList& artist, albumList& album, indexSet& result, indexSet& delAlbArray)
{
    system("cls");
    cout << setw(30) << "Delete Album " << endl;
    int idx;
    char answer;
    do{
        searchArtist(ArtFile, artist, result);
        if (result.indexes.empty()){
            printError(4);
            system("pause");
            return;
        }
    }while(result.indexes.empty());
    idx = selectArtist(ArtFile, artist, result, "Delete");
    cout << "Do you want to remove all the albums of this artist?(Y/N): ";
    cin >> answer;
    if( answer == 'y' || answer == 'Y' ){
        for(size_t i = 0; i < album.albList.size(); i++){
            removeArtistAllAlbums(ArtFile, AlbFile, artist, album, delAlbArray, i);
        }
        cout << "\n\t All Albums Successfully Removed!\n\n";
        system("pause");
    }
    else{
        idx = selectAlbum(AlbFile, artist, album, result, idx, "Delete");
        if( idx == -1){
            return;
        }
        cout << "Are you sure?(Y/N): ";
        cin >> answer;
        if( answer == 'y' || answer == 'Y' ){
            removeAlbum(AlbFile, album, delAlbArray, idx);
        }
        else{
            cout << "\n\t Failed!\n\n";
            system("pause");
            return;
        }
    }
}

//70
void removeAlbum(std::fstream& AlbFile, albumList& album, indexSet& delAlbArray, int idx)
{
    int pos;
    AlbumFile BLANK_ALBUM_FILE = {"-1", "-1", "", "", "", ""};
    AlbFile.seekp(album.albList[idx].pos, ios::beg);
    pos = AlbFile.tellp();
    AlbFile.write((char*)&BLANK_ALBUM_FILE, sizeof(AlbumFile));
    album.albList[idx].albumId = BLANK_ALBUM_FILE.albumIds;
    album.albList[idx].artistId = BLANK_ALBUM_FILE.artistIdRefs;
    album.albList[idx].title = BLANK_ALBUM_FILE.titles;
    album.albList[idx].pos = pos;
    delAlbArray.indexes.push_back(idx);
    cout << "\n\t Successfully Removed.\n\n";
    system("pause");
}

//71
bool searchAlbumByTitle(std::fstream& AlbFile, const albumList& album, indexSet& result, const std::string& title)
{
    result.indexes.clear();
    AlbumFile albFile;
    for (size_t i = 0; i < album.albList.size(); i++) {
        AlbFile.seekg(album.albList[i].pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        std::string albTitle(albFile.titles);
        if (albTitle.find(title) == 0) {
            result.indexes.push_back(i);
        }
    }
    return !result.indexes.empty();
}

bool searchAlbumByDateRange(std::fstream& AlbFile, const albumList& album, indexSet& result, unsigned int startDay, unsigned int startMonth, unsigned int startYear, unsigned int endDay, unsigned int endMonth, unsigned int endYear)
{
    result.indexes.clear();
    AlbumFile albFile;
    for (size_t i = 0; i < album.albList.size(); i++) {
        AlbFile.seekg(album.albList[i].pos, ios::beg);
        AlbFile.read((char*)&albFile, sizeof(albFile));
        std::string dateStr(albFile.datePublished);
        // Parse date DD/MM/YYYY
        unsigned int day = (dateStr[0] - '0') * 10 + (dateStr[1] - '0');
        unsigned int month = (dateStr[3] - '0') * 10 + (dateStr[4] - '0');
        unsigned int year = (dateStr[6] - '0') * 1000 + (dateStr[7] - '0') * 100 + (dateStr[8] - '0') * 10 + (dateStr[9] - '0');
        if (year > startYear || (year == startYear && (month > startMonth || (month == startMonth && day >= startDay)))) {
            if (year < endYear || (year == endYear && (month < endMonth || (month == endMonth && day <= endDay)))) {
                result.indexes.push_back(i);
            }
        }
    }
    return !result.indexes.empty();
}

void advancedSearchAlbums(std::fstream& AlbFile, const albumList& album, indexSet& result)
{
    system("cls");
    cout << "\nAdvanced Search Options:\n";
    cout << "1. Search by Album Title\n";
    cout << "2. Search by Date Range\n";
    int choice;
    cout << "Enter choice: ";
    cin >> choice;
    cin.ignore(INT_MAX, '\n');
    if (choice == 1) {
        std::string title;
        cout << "Enter album title prefix: ";
        getline(cin, title);
        if (searchAlbumByTitle(AlbFile, album, result, title)) {
            displayAlbumSearchResult(AlbFile, album, result);
        } else {
            printError(4);
        }
    } else if (choice == 2) {
        unsigned int startDay, startMonth, startYear, endDay, endMonth, endYear;
        cout << "Enter start date (DD/MM/YYYY): ";
        cin >> startDay;
        cin.ignore();
        cin >> startMonth;
        cin.ignore();
        cin >> startYear;
        cout << "Enter end date (DD/MM/YYYY): ";
        cin >> endDay;
        cin.ignore();
        cin >> endMonth;
        cin.ignore();
        cin >> endYear;
        if (searchAlbumByDateRange(AlbFile, album, result, startDay, startMonth, startYear, endDay, endMonth, endYear)) {
            displayAlbumSearchResult(AlbFile, album, result);
        } else {
            printError(4);
        }
    } else {
        cout << "Invalid choice.\n";
    }
    cout << endl << endl;
    system("pause");
}

void farewell() {
    system("cls");
    cout << "\n\n\n\n\n\n\n\t\t\t\tTHANK YOU FOR USING ALBUM MANAGEMENT SYSTEM!\n\n\n\n\n\n\n";
    system("pause");
}

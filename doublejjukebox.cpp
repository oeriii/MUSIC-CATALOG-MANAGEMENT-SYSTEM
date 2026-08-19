#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace std;

const int MAX_SONGS = 100;
const int STACK_MAX = 50;
const int MAX_LIMIT = 100;

struct ReleaseInfo {
    string albumName;
    int releaseYear;
};

// Song struct
struct Song {
    string title;
    string artist;
    ReleaseInfo release; // nested structure
    string genre;
    int duration;
};

typedef Song SongRecord;

// Album struct
struct Album {
    string albumName;
    string albumArtist;
    int releaseYear;
    vector<Song> tracklist;
};

// Tree node for the Album BST
struct AlbumTreeNode {
    Album data;
    AlbumTreeNode* left;
    AlbumTreeNode* right;
};

// Converts string to lowercase (from main.cpp)
string toLower(string str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// INPUT VALIDATION HELPERS

const vector<string> GENRE_OPTIONS = {
    "Pop", "R&B / Soul", "Hip-Hop / Rap", "Alternative / Indie",
    "Electronic / Dance", "Rock", "Acoustic / Folk", "OPM", "Other"
};

// ask the user to type something that isn't blank or invalid
string promptRequiredField(const string &label) {
    string value;
    while (true) {
        cout << "Enter " << label << " (required): ";
        getline(cin, value);

        size_t start = value.find_first_not_of(" \t");
        if (start == string::npos) {
            cout << "  -> " << label << " cannot be empty. Please try again.\n";
            continue;
        }
        return value;
    }
}

string promptOptionalField(const string &label) {
    cout << "Enter " << label << " (optional, press Enter to skip): ";
    string value;
    getline(cin, value);
    return value;
}

int promptOptionalYear() {
    while (true) {
        cout << "Enter Release Year (optional, press Enter to skip): ";
        string line;
        getline(cin, line);

        if (line.empty()) return 0;

        bool allDigits = (line.find_first_not_of("0123456789") == string::npos);
        if (allDigits) {
            try {
                return stoi(line);
            }
            catch (...) {
            }
        }

        cout << " Please enter a valid year (digits only) or press Enter to skip.\n";
    }
}

int promptValidatedChoice(int minVal, int maxVal, const string &promptText) {
    string line;
    while (true) {
        cout << promptText;
        getline(cin, line);

        try {
            size_t idx;
            int choice = stoi(line, &idx);
            if (choice >= minVal && choice <= maxVal) return choice;
        }
        catch (...) {
        }

        cout << "  Please Enter a valid Entry. Enter a number between "
             << minVal << " and " << maxVal << ".\n";
    }
}

string chooseGenre() {
    cout << "\nSelect Genre:\n";
    for (size_t i = 0; i < GENRE_OPTIONS.size(); i++) {
        cout << "  [" << (i + 1) << "] " << GENRE_OPTIONS[i] << "\n";
    }

    while (true) {
        cout << "Enter choice (1-" << GENRE_OPTIONS.size() << "): ";
        string line;
        getline(cin, line);

        int choice = -1;
        try {
            choice = stoi(line);
        }
        catch (...) {
            choice = -1;
        }

        if (choice >= 1 && choice <= (int)GENRE_OPTIONS.size()) {
            return GENRE_OPTIONS[choice - 1];
        }
        cout << "  Please Enter a valid choice.\n";
    }
}

int parseDurationString(const string &input, bool &valid) {
    valid = false;

    size_t colonPos = input.find(':');
    if (colonPos == string::npos) return 0;

    string minPart = input.substr(0, colonPos);
    string secPart = input.substr(colonPos + 1);

    if (minPart.empty() || secPart.empty()) return 0;
    if (minPart.find_first_not_of("0123456789") != string::npos) return 0;
    if (secPart.find_first_not_of("0123456789") != string::npos) return 0;

    int minutes = stoi(minPart);
    int seconds = stoi(secPart);
    if (seconds >= 60) return 0;

    valid = true;
    return (minutes * 60) + seconds;
}

int promptDuration() {
    while (true) {
        cout << "Enter Duration (format M:SS, e.g. 3:45): ";
        string line;
        getline(cin, line);

        bool valid = false;
        int seconds = parseDurationString(line, valid);
        if (valid) return seconds;

        cout << " Invalid format. Use M:SS, e.g. 3:45 (seconds must be 00-59).\n";
    }
}

string formatDuration(int totalSeconds) {
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    ostringstream oss;
    oss << minutes << ":" << setw(2) << setfill('0') << seconds;
    return oss.str();
}

// SECTION 1: CATALOG.CPP -- Song array add/display/sort/search

void addSong(Song songs[], int &count);
void displayAllSongs(Song songs[], int count);
void selectionSortByDuration(Song songs[], int count);
void searchSongByTitle(Song songs[], int count);
void selectionSortByTitle(Song songs[], int count);
void binarySearchByTitle(Song songs[], int count);

void addSong(Song songs[], int &count) {
    if (count >= MAX_SONGS) {
        cout << "Error: The catalog is full (Max " << MAX_SONGS << " songs)." << endl;
        return;
    }

    cout << "\n--- Add a New Song ---" << endl;

    Song newSong;
    newSong.title = promptRequiredField("Title");
    newSong.artist = promptRequiredField("Artist");
    newSong.release.albumName = promptOptionalField("Album");
    newSong.release.releaseYear = promptOptionalYear();
    newSong.genre = chooseGenre();
    newSong.duration = promptDuration();

    songs[count] = newSong;
    count++;

    cout << "\nSong added successfully!" << endl;
}

void displayAllSongs(Song songs[], int count) {
    if (count == 0) {
        cout << "\nNo songs in the catalog." << endl;
        return;
    }

    cout << "\n--- All Songs in Catalog ---" << endl;

    cout << left << setw(30) << "Title"
         << setw(25) << "Artist"
         << setw(25) << "Album"
         << setw(22) << "Genre"
         << setw(10) << "Year"
         << setw(10) << "Duration" << endl;

    cout << setfill('-') << setw(122) << "-" << setfill(' ') << endl;

    for (int i = 0; i < count; i++) {
        cout << left << setw(30) << songs[i].title
             << setw(25) << songs[i].artist
             << setw(25) << songs[i].release.albumName
             << setw(22) << songs[i].genre
             << setw(10) << songs[i].release.releaseYear
             << setw(10) << formatDuration(songs[i].duration) << endl;
    }
}

void selectionSortByDuration(Song songs[], int count) {
    if (count <= 1) {
        cout << "\nNot enough songs to sort." << endl;
        return;
    }

    for (int i = 0; i < count - 1; i++) {
        int minIndex = i;

        for (int j = i + 1; j < count; j++) {
            if (songs[j].duration < songs[minIndex].duration) {
                minIndex = j;
            }
        }

        if (minIndex != i) {
            Song temp = songs[i];
            songs[i] = songs[minIndex];
            songs[minIndex] = temp;
        }
    }

    cout << "\nCatalog has been successfully sorted by duration (ascending)." << endl;
}

void searchSongByTitle(Song songs[], int count) {
    if (count == 0) {
        cout << "\nCatalog is empty. Cannot search." << endl;
        return;
    }

    string searchTitle;
    cout << "\nEnter Song Title to search: ";
    getline(cin, searchTitle);

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (songs[i].title == searchTitle) {
            cout << "\n--- Song Found ---" << endl;
            cout << "Title:    " << songs[i].title << endl;
            cout << "Artist:   " << songs[i].artist << endl;
            cout << "Album:    " << songs[i].release.albumName << endl;
            cout << "Genre:    " << songs[i].genre << endl;
            cout << "Year:     " << songs[i].release.releaseYear << endl;
            cout << "Duration: " << formatDuration(songs[i].duration) << endl;

            found = true;
            break;
        }
    }

    if (!found) {
        cout << "\nSong not found." << endl;
    }
}

void selectionSortByTitle(Song songs[], int count) {
    if (count <= 1) return;

    for (int i = 0; i < count - 1; i++) {
        int minIndex = i;

        for (int j = i + 1; j < count; j++) {
            if (songs[j].title < songs[minIndex].title) {
                minIndex = j;
            }
        }

        if (minIndex != i) {
            Song temp = songs[i];
            songs[i] = songs[minIndex];
            songs[minIndex] = temp;
        }
    }
}

void binarySearchByTitle(Song songs[], int count) {
    if (count == 0) {
        cout << "\nCatalog is empty. Cannot search." << endl;
        return;
    }

    cout << "\n--- Available Songs (sorted by title) ---" << endl;
    for (int i = 0; i < count; i++) {
        cout << "  " << songs[i].title << " - " << songs[i].artist << endl;
    }

    string searchTitle;
    cout << "\nEnter Song Title to search (Binary Search): ";
    getline(cin, searchTitle);

    int low = 0, high = count - 1, foundIndex = -1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (songs[mid].title == searchTitle) {
            foundIndex = mid;
            break;
        } else if (songs[mid].title < searchTitle) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    if (foundIndex != -1) {
        cout << "\n--- Song Found (Binary Search) ---" << endl;
        cout << "Title:    " << songs[foundIndex].title << endl;
        cout << "Artist:   " << songs[foundIndex].artist << endl;
        cout << "Album:    " << songs[foundIndex].release.albumName << endl;
        cout << "Genre:    " << songs[foundIndex].genre << endl;
        cout << "Year:     " << songs[foundIndex].release.releaseYear << endl;
        cout << "Duration: " << formatDuration(songs[foundIndex].duration) << endl;
    } else {
        cout << "\nSong not found." << endl;
    }
}

// SECTION 2: TREE.CPP -- Recently-played stack and Genre BST

//dito noy lagay recently played stack
class PlayedHistoryStack {
private:
    Song history[STACK_MAX];
    int topIndex;

public:
    PlayedHistoryStack() {
        topIndex = -1;
    }

    bool isFull() {
        return topIndex == STACK_MAX - 1;
    }

    bool isEmpty() {
        return topIndex == -1;
    }

    void pushPlayed(Song playedSong) {
        if (isFull()) {
            cout << "History stack overflow! Cannot add more songs.\n";
            return;
        }
        topIndex++;
        history[topIndex] = playedSong;
    }

    Song popPlayed() {
        if (isEmpty()) {
            cout << "History is empty!\n";
            return Song{"", "", {"", 0}, "", 0};
        }
        Song lastPlayed = history[topIndex];
        topIndex--;
        return lastPlayed;
    }

    void displayRecentlyPlayed() {
        if (isEmpty()) {
            cout << "No recently played songs.\n";
            return;
        }
        cout << "--- Recently Played History (Latest First) ---\n";
        for (int i = topIndex; i >= 0; i--) {
            cout << history[i].title << " by " << history[i].artist << "\n";
        }
    }
};

//Genre/Artist Hierarchy
struct GenreTreeNode {
    Song songData;
    GenreTreeNode* left;
    GenreTreeNode* right;

    GenreTreeNode(Song song) {
        songData = song;
        left = nullptr;
        right = nullptr;
    }
};

class GenreTree {
private:
    GenreTreeNode* root;

    GenreTreeNode* insertHelper(GenreTreeNode* node, Song song) {
        if (node == nullptr) {
            return new GenreTreeNode(song);
        }

        if (song.genre < node->songData.genre) {
            node->left = insertHelper(node->left, song);
        } else {
            node->right = insertHelper(node->right, song);
        }
        return node;
    }

    void inOrderHelper(GenreTreeNode* node) {
        if (node == nullptr) return;

        inOrderHelper(node->left);  

        cout << "[" << node->songData.genre << "] "
             << node->songData.title << " - "
             << node->songData.artist << "\n";

        inOrderHelper(node->right); 
    }

public:
    GenreTree() {
        root = nullptr;
    }

    // insertgenre dito 
    void insertGenre(Song song) {
        root = insertHelper(root, song);
    }

    // display tree alphabetically by genre
    void inOrderDisplay() {
        if (root == nullptr) {
            cout << "The genre tree is empty.\n";
            return;
        }
        cout << "--- Songs Sorted Alphabetically by Genre ---\n";
        inOrderHelper(root);
    }
};


// main stuff
// (function bodies unchanged from main.cpp, aside from TreeNode ->
// AlbumTreeNode rename, and Song's album/year now going through
// song.release.albumName / song.release.releaseYear)


AlbumTreeNode* insertAlbum(AlbumTreeNode* node, const Album& newAlbum) {
    if (node == nullptr) {
        AlbumTreeNode* newNode = new AlbumTreeNode;
        newNode->data = newAlbum;
        newNode->left = newNode->right = nullptr;
        return newNode;
    }

    string newName = toLower(newAlbum.albumName);
    string currentName = toLower(node->data.albumName);

    if (newName < currentName) {
        node->left = insertAlbum(node->left, newAlbum);
    }
    else if (newName > currentName) {
        node->right = insertAlbum(node->right, newAlbum);
    }
    else {
        string newArtist = toLower(newAlbum.albumArtist);
        string currentArtist = toLower(node->data.albumArtist);

        if (newArtist < currentArtist)
            node->left = insertAlbum(node->left, newAlbum);
        else if (newArtist > currentArtist)
            node->right = insertAlbum(node->right, newAlbum);
    }
    return node;
}

void clearTree(AlbumTreeNode*& node) {
    if (node == nullptr) return;
    clearTree(node->left);
    clearTree(node->right);
    delete node;
    node = nullptr;
}

// dito dinisplay ung treenode nung album
void displayAlbumsHelper(AlbumTreeNode* node) {
    if (node == nullptr) return;
    displayAlbumsHelper(node->left);
    cout << "\"" << node->data.albumName << "\" by " << node->data.albumArtist
         << " (" << node->data.releaseYear << ") - "
         << node->data.tracklist.size() << " track(s)\n";
    displayAlbumsHelper(node->right);
}

void displayAlbums(AlbumTreeNode* root) {
    if (root == nullptr) {
        cout << "\nNo albums in the catalog.\n";
        return;
    }
    cout << "\n--- Albums (sorted alphabetically) ---\n";
    displayAlbumsHelper(root);
}

void saveSong(ofstream& outFile, const Song& song) {
    outFile << song.title << "|"
            << song.artist << "|"
            << song.release.albumName << "|"
            << song.genre << "|"
            << song.release.releaseYear << "|"
            << song.duration << "\n";
}

void saveAlbum(ofstream& outFile, const Album& album) {
    outFile << album.albumName << "|"
            << album.albumArtist << "|"
            << album.releaseYear << "|"
            << album.tracklist.size() << "\n";
    for (const Song& song : album.tracklist) {
        saveSong(outFile, song);
    }
}

void saveToFile(AlbumTreeNode* node, ofstream& outFile) {
    if (node == nullptr) return;
    saveAlbum(outFile, node->data);
    saveToFile(node->left, outFile);
    saveToFile(node->right, outFile);
}

void loadFromFile(AlbumTreeNode*& musicCatalog, const string& fileName) {
    ifstream inFile(fileName);
    if (!inFile.is_open()) {
        return;
    }

    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;

        try {
            stringstream ss(line);
            Album tempAlbum;
            string trackCountStr, yearStr;

            if (!getline(ss, tempAlbum.albumName, '|') ||
                !getline(ss, tempAlbum.albumArtist, '|') ||
                !getline(ss, yearStr, '|') ||
                !getline(ss, trackCountStr, '|')) {
                continue;
            }

            tempAlbum.releaseYear = stoi(yearStr);
            int trackCount = stoi(trackCountStr);
            bool albumCorrupted = false;

            for (int i = 0; i < trackCount; ++i) {
                string songLine;
                if (!getline(inFile, songLine)) {
                    albumCorrupted = true;
                    break;
                }
                stringstream songSS(songLine);
                Song tempSong;
                string songAlbumStr, songYearStr, durationStr;

                if (!getline(songSS, tempSong.title, '|') ||
                    !getline(songSS, tempSong.artist, '|') ||
                    !getline(songSS, songAlbumStr, '|') ||
                    !getline(songSS, tempSong.genre, '|') ||
                    !getline(songSS, songYearStr, '|') ||
                    !getline(songSS, durationStr, '|')) {
                    albumCorrupted = true;
                    break;
                }

                tempSong.release.albumName = songAlbumStr;
                tempSong.release.releaseYear = stoi(songYearStr);
                tempSong.duration = stoi(durationStr);
                tempAlbum.tracklist.push_back(tempSong);
            }

            if (!albumCorrupted) {
                musicCatalog = insertAlbum(musicCatalog, tempAlbum);
            }
        }
        catch (const invalid_argument&) {
            continue;
        }
        catch (const out_of_range&) {
            continue;
        }
    }
    inFile.close();
}

void saveCatalogToFile(Song songs[], int count, const string& fileName) {
    ofstream outFile(fileName);
    if (!outFile.is_open()) return;

    outFile << count << "\n";
    for (int i = 0; i < count; i++) {
        saveSong(outFile, songs[i]);
    }
    outFile.close();
}

void loadCatalogFromFile(Song songs[], int& count, const string& fileName) {
    ifstream inFile(fileName);
    if (!inFile.is_open()) return;

    string countLine;
    if (!getline(inFile, countLine)) return;

    int savedCount = 0;
    try {
        savedCount = stoi(countLine);
    }
    catch (const invalid_argument&) {
        return;
    }
    catch (const out_of_range&) {
        return;
    }

    for (int i = 0; i < savedCount && count < MAX_SONGS; i++) {
        string songLine;
        if (!getline(inFile, songLine)) break;

        stringstream songSS(songLine);
        Song tempSong;
        string albumStr, yearStr, durationStr;

        if (!getline(songSS, tempSong.title, '|') ||
            !getline(songSS, tempSong.artist, '|') ||
            !getline(songSS, albumStr, '|') ||
            !getline(songSS, tempSong.genre, '|') ||
            !getline(songSS, yearStr, '|') ||
            !getline(songSS, durationStr, '|')) {
            continue;
        }

        try {
            tempSong.release.albumName = albumStr;
            tempSong.release.releaseYear = stoi(yearStr);
            tempSong.duration = stoi(durationStr);
        }
        catch (const invalid_argument&) {
            continue;
        }
        catch (const out_of_range&) {
            continue;
        }

        songs[count] = tempSong;
        count++;
    }

    inFile.close();
}

void seedTopQuarantineSongs(Song songs[], int &count) {
    struct SeedSong {
        string title, artist, album, genre;
        int year, durationSeconds;
    };

    SeedSong seeds[] = {
        {"Mariposa",        "Peach Tree Rascals", "Mariposa",           "Alternative / Indie", 2019, 199},
        {"Sunday Best",     "Surfaces",           "Where the Light Is", "Pop",                  2019, 176},
        {"Blinding Lights", "The Weeknd",         "After Hours",        "Pop",                  2019, 200},
        {"Toosie Slide",    "Drake",              "Dark Lane Demo Tapes","Hip-Hop / Rap",        2020, 247},
        {"Adore You",       "Harry Styles",       "Fine Line",          "Pop",                  2019, 207}
    };

    for (const SeedSong &s : seeds) {
        if (count >= MAX_SONGS) break;

        Song song;
        song.title = s.title;
        song.artist = s.artist;
        song.release.albumName = s.album;
        song.release.releaseYear = s.year;
        song.genre = s.genre;
        song.duration = s.durationSeconds;

        songs[count] = song;
        count++;
    }
}

//Starter Album (Starboy by The Weeknd)

void seedStarboyAlbum(AlbumTreeNode*& musicCatalog) {
    Album starboy;
    starboy.albumName = "Starboy";
    starboy.albumArtist = "The Weeknd";
    starboy.releaseYear = 2016;

    struct SeedTrack { string title; int durationSeconds; };
    SeedTrack tracks[] = {
        {"Starboy",             230},
        {"Party Monster",       249},
        {"Stargirl Interlude",  123},
        {"False Alarm",         220},
        {"Reminder",            227},
        {"Rockin'",             195},
        {"Secrets",             292},
        {"True Colors",         262},
        {"Sidewalks",           246},
        {"Six Feet Under",      213},
        {"Love to Lay",         200},
        {"A Lonely Night",      243},
        {"Attention",           213},
        {"Ordinary Life",       212},
        {"Nothing Without You", 209},
        {"All I Know",          321},
        {"Die For You",         260},
        {"I Feel It Coming",    269}
    };

    for (const SeedTrack &t : tracks) {
        Song s;
        s.title = t.title;
        s.artist = starboy.albumArtist;
        s.release.albumName = starboy.albumName;
        s.release.releaseYear = starboy.releaseYear;
        s.genre = "R&B / Soul";
        s.duration = t.durationSeconds;
        starboy.tracklist.push_back(s);
    }

    musicCatalog = insertAlbum(musicCatalog, starboy);
}

void addAlbum(AlbumTreeNode*& musicCatalog) {
    Album newAlbum;

    cout << "\n--- Add a New Album ---" << endl;
    newAlbum.albumName = promptRequiredField("Album Name");
    newAlbum.albumArtist = promptRequiredField("Album Artist");
    newAlbum.releaseYear = promptOptionalYear();

    int trackCount = promptValidatedChoice(1, 50, "How many tracks on this album? (1-50): ");

    for (int i = 0; i < trackCount; i++) {
        Song s;
        cout << "\nTrack " << (i + 1) << ":" << endl;
        s.title = promptRequiredField("Title");
        s.artist = newAlbum.albumArtist;
        s.release.albumName = newAlbum.albumName;
        s.release.releaseYear = newAlbum.releaseYear;
        s.genre = chooseGenre();
        s.duration = promptDuration();
        newAlbum.tracklist.push_back(s);
    }

    musicCatalog = insertAlbum(musicCatalog, newAlbum);
    cout << "\nAlbum added successfully!" << endl;
}



const int BOX_WIDTH = 57;
string centerText(const string& text, int width) {
    if ((int)text.size() >= width) return text.substr(0, width);
    int left = (width - (int)text.size()) / 2;
    int right = width - (int)text.size() - left;
    return string(left, ' ') + text + string(right, ' ');
}

void printBoxLine(const string& text) {
    cout << "|" << left << setw(BOX_WIDTH) << text << "|" << endl;
}

void printBoxDivider(char fill = '-') {
    cout << "+" << string(BOX_WIDTH, fill) << "+" << endl;
}

string twoCol(const string& a, const string& b) {
    ostringstream oss;
    oss << "  " << left << setw(27) << a << " " << b;
    return oss.str();
}

//Fake boot sequence
void loadingSequence() {
    cout << "\n";
    string steps[] = {
        "Spinning up the turntable",
        "Waking the genre tree",
        "Dusting off the record crates",
        "Tuning the equalizer",
        "Reticulating playlists"
    };

    for (const string& step : steps) {
        cout << "  > " << left << setw(32) << step << flush;
        for (int i = 0; i < 3; i++) {
            this_thread::sleep_for(chrono::milliseconds(120));
            cout << "." << flush;
        }
        cout << " OK" << endl;
    }

    cout << endl;
    const int barWidth = 40;
    for (int i = 0; i <= barWidth; i++) {
        cout << "\r  [";
        for (int j = 0; j < barWidth; j++) {
            cout << (j < i ? '#' : (j == i ? '>' : '-'));
        }
        cout << "] " << setw(3) << (i * 100 / barWidth) << "%" << flush;
        this_thread::sleep_for(chrono::milliseconds(18));
    }
    cout << "\n\n  System ready.\n";
    this_thread::sleep_for(chrono::milliseconds(400));
}

// ASCII logo
const int LOGO_WIDTH = 50;

void printLogoLine(const string& text) {
    cout << "|" << centerText(text, LOGO_WIDTH) << "|\n";
}

void printLogo() {
    cout << "\n";
    cout << "   .-------------------------------------------.\n";
    cout << "  /  |||   |  ||  |||||   ||||   |  ||   |||||   \\\n";
    cout << " /___|_|___|__||__|___|___||_||__|__||___|_|_|____\\\n";
    printLogoLine("");
    printLogoLine("Double James Jukebox");
    printLogoLine("= MUSIC CATALOG MANAGEMENT SYSTEM =");
    printLogoLine("");
    cout << "|" << string(LOGO_WIDTH, '_') << "|\n";
    cout << "   (o)" << string(LOGO_WIDTH - 12, ' ') << "(o)\n";
    cout << "\n";
}

void printMenu() {
    printBoxDivider('=');
    printBoxLine(" SONG CATALOG");
    printBoxDivider();
    printBoxLine(twoCol("[1] Add Song", "[2] Display All Songs"));
    printBoxLine(twoCol("[3] Sort by Duration", "[4] Search by Title"));
    printBoxLine(twoCol("[5] Binary Search by Title", ""));

    printBoxDivider('=');
    printBoxLine(" NOW PLAYING");
    printBoxDivider();
    printBoxLine(twoCol("[6] Play a Song", "[7] Recently Played"));
    printBoxLine(twoCol("[8] Skip Back", ""));

    printBoxDivider('=');
    printBoxLine(" GENRE TREE");
    printBoxDivider();
    printBoxLine(twoCol("[9] Show Songs by Genre (A-Z)", ""));

    printBoxDivider('=');
    printBoxLine(" ALBUMS");
    printBoxDivider();
    printBoxLine(twoCol("[10] Add Album", "[11] Display Albums"));

    printBoxDivider('=');
    printBoxLine(twoCol("[12] Save & Exit", ""));
    printBoxDivider('=');
}

void waitBackToMenu() {
    promptValidatedChoice(1, 1, "\n  [1] Back to Main Menu\n  >> Enter your choice: ");
}

void addSongMenu(Song catalog[], int &songCount, GenreTree &genreTree) {
    bool keepAdding = true;

    while (keepAdding) {
        addSong(catalog, songCount);
        if (songCount > 0) {
            genreTree.insertGenre(catalog[songCount - 1]);
        }

        cout << "\n  [1] Add Another Song\n  [2] Back to Main Menu\n";
        int choice = promptValidatedChoice(1, 2, "  >> Enter your choice (1-2): ");
        if (choice == 2) {
            keepAdding = false;
        }
    }
}

//play song
void playSongMenu(Song catalog[], int songCount, PlayedHistoryStack &historyStack) {
    if (songCount == 0) {
        cout << "\nCatalog is empty. Add a song first." << endl;
        return;
    }

    bool keepPlaying = true;

    while (keepPlaying) {
        cout << "\n--- Available Songs ---" << endl;
        cout << "  [1] Back to Main Menu" << endl;
        for (int i = 0; i < songCount; i++) {
            cout << "  [" << (i + 2) << "] " << catalog[i].title
                 << " - " << catalog[i].artist << endl;
        }

        int pick = promptValidatedChoice(1, songCount + 1, "\nEnter your choice: ");
        if (pick == 1) {
            keepPlaying = false;
            break;
        }

        int songIndex = pick - 2;
        historyStack.pushPlayed(catalog[songIndex]);
        cout << "Now playing: " << catalog[songIndex].title
             << " by " << catalog[songIndex].artist << endl;

        cout << "\n  [1] Play Another Song\n  [2] Return to Main Menu\n";
        int subChoice = promptValidatedChoice(1, 2, "  >> Enter your choice (1-2): ");
        if (subChoice == 2) {
            keepPlaying = false;
        }
    }
}

int main() {
    Song catalog[MAX_SONGS];
    int songCount = 0;

    string catalogFileName = "catalog.txt";
    loadCatalogFromFile(catalog, songCount, catalogFileName);

    if (songCount == 0) {
        seedTopQuarantineSongs(catalog, songCount);
    }

    PlayedHistoryStack historyStack;
    GenreTree genreTree;

    for (int i = 0; i < songCount; i++) {
        genreTree.insertGenre(catalog[i]);
    }

    AlbumTreeNode* musicCatalog = nullptr;
    string saveFileName = "songs.txt";
    loadFromFile(musicCatalog, saveFileName);

    if (musicCatalog == nullptr) {
        seedStarboyAlbum(musicCatalog);
    }

    loadingSequence();
    printLogo();

    int choice;

    do {
        printMenu();
        choice = promptValidatedChoice(1, 12, "\n  >> Enter your choice (1-12): ");

        switch (choice) {
            case 1: {
                addSongMenu(catalog, songCount, genreTree);
                break;
            }
            case 2:
                displayAllSongs(catalog, songCount);
                waitBackToMenu();
                break;
            case 3:
                selectionSortByDuration(catalog, songCount);
                waitBackToMenu();
                break;
            case 4:
                searchSongByTitle(catalog, songCount);
                waitBackToMenu();
                break;
            case 5: {
                cout << "\n(Sorting catalog alphabetically by title first, "
                        "as required for binary search...)" << endl;
                selectionSortByTitle(catalog, songCount);
                binarySearchByTitle(catalog, songCount);
                waitBackToMenu();
                break;
            }
            case 6: {
                playSongMenu(catalog, songCount, historyStack);
                break;
            }
            case 7:
                historyStack.displayRecentlyPlayed();
                waitBackToMenu();
                break;
            case 8: {
                Song skipped = historyStack.popPlayed();
                if (!skipped.title.empty()) {
                    cout << "Skipped back from: " << skipped.title << endl;
                }
                waitBackToMenu();
                break;
            }
            case 9:
                genreTree.inOrderDisplay();
                waitBackToMenu();
                break;
            case 10:
                addAlbum(musicCatalog);
                waitBackToMenu();
                break;
            case 11:
                displayAlbums(musicCatalog);
                waitBackToMenu();
                break;
            case 12:
                cout << "Saving and exiting the program. Goodbye!" << endl;
                break;
            default:
                cout << "Please Enter a valid entry. Select a number between 1 and 12." << endl;
        }
    } while (choice != 12);

    saveCatalogToFile(catalog, songCount, catalogFileName);

    ofstream outFile(saveFileName);
    if (outFile.is_open()) {
        saveToFile(musicCatalog, outFile);
        outFile.close();
    }

    clearTree(musicCatalog);
    return 0;
}
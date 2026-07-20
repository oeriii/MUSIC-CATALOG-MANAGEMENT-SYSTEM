// =============================================================================
// Double James Jukebox -- Music Catalog Management System
// Merged from: catalog.cpp (song catalog CRUD/sort/search)
//              tree.cpp    (play history stack + genre BST)
//              main.cpp    (album BST + file save/load)
//
// NOTE: This file combines all three teammates' code into one program.
// Each teammate's functions/classes were kept as close to original as
// possible. The only changes made were:
//   - Only ONE "Song" struct now exists (catalog.cpp's version, since it
//     has all the fields everyone needs: title, artist, album, genre,
//     year, duration). tree.cpp's Song/AlbumInfo were removed and its
//     functions now just use this shared Song struct.
//   - main.cpp's album "TreeNode" was renamed to "AlbumTreeNode" and
//     tree.cpp's genre "TreeNode" was renamed to "GenreTreeNode" so the
//     two different tree node types don't clash with each other.
//   - Each file's own test/menu main() was removed and replaced with a
//     single combined menu below.
//   - Song now bundles album name + release year into a nested
//     ReleaseInfo structure (rubric requires the primary array record
//     to contain a nested structure), with a typedef alias on Song.
//   - Added a manual Binary Search (by title) alongside the existing
//     linear search, with a mandatory sort-by-title step before it runs.
//   - The raw Song catalog[] array now has its own save/load to disk,
//     separate from the Album tree ledger, so it actually persists.
// =============================================================================

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

const int MAX_SONGS = 100;   // from catalog.cpp
const int STACK_MAX = 50;    // from tree.cpp
const int MAX_LIMIT = 100;   // from main.cpp

// =============================================================================
// SHARED STRUCTURES
// =============================================================================

// Nested structure holding a song's release details (album + year).
// Bundling these together lets Song demonstrate structure composition,
// as required by the rubric ("a structure variable defined inside
// another structure").
struct ReleaseInfo {
    string albumName;
    int releaseYear;
};

// Song struct (from catalog.cpp) -- used everywhere now.
// Contains a nested ReleaseInfo structure (release) to map the
// album/year sub-attributes, instead of flat album/year fields.
struct Song {
    string title;
    string artist;
    ReleaseInfo release; // nested structure
    string genre;
    int duration;
};

// Typedef alias for the primary record type, as required by the rubric.
typedef Song SongRecord;

// Album struct (from main.cpp)
struct Album {
    string albumName;
    string albumArtist;
    int releaseYear;
    vector<Song> tracklist;
};

// Tree node for the Album BST (from main.cpp, renamed from TreeNode)
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

// =============================================================================
// INPUT VALIDATION HELPERS
// (Title/Artist are required fields. Album/Year are optional. Duration
// must be entered as M:SS and is stored internally as total seconds.
// Genre is chosen from a fixed list instead of free-typed text.)
// =============================================================================

const vector<string> GENRE_OPTIONS = {
    "Pop", "R&B / Soul", "Hip-Hop / Rap", "Alternative / Indie",
    "Electronic / Dance", "Rock", "Acoustic / Folk", "OPM", "Other"
};

// Keeps asking until the user types something that isn't blank/whitespace.
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

// Blank input is allowed here; just returns whatever was typed (or "").
string promptOptionalField(const string &label) {
    cout << "Enter " << label << " (optional, press Enter to skip): ";
    string value;
    getline(cin, value);
    return value;
}

// Optional year: blank -> 0. If the user types something, it must be
// all digits, otherwise it's rejected and treated as skipped.
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
                // falls through to the retry message below
            }
        }

        cout << "  -> Invalid year. Digits only, or press Enter to skip.\n";
    }
}

// Generic validated-choice prompt. Loops until the user enters a number
// within [minVal, maxVal]. Used for the main menu and any submenu.
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
            // falls through to the retry message below
        }

        cout << "  -> Invalid choice. Please enter a number between "
             << minVal << " and " << maxVal << ".\n";
    }
}

// User picks a genre by number instead of typing it manually.
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
        cout << "  -> Invalid choice, try again.\n";
    }
}

// Parses "M:SS" into total seconds. Sets valid=false on any bad format
// (missing colon, non-digit characters, or seconds >= 60).
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

// Keeps asking until a valid M:SS duration is entered. Returns seconds.
int promptDuration() {
    while (true) {
        cout << "Enter Duration (format M:SS, e.g. 3:45): ";
        string line;
        getline(cin, line);

        bool valid = false;
        int seconds = parseDurationString(line, valid);
        if (valid) return seconds;

        cout << "  -> Invalid format. Use M:SS, e.g. 3:45 (seconds must be 00-59).\n";
    }
}

// Converts total seconds back into "M:SS" for display, e.g. 225 -> "3:45"
string formatDuration(int totalSeconds) {
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    ostringstream oss;
    oss << minutes << ":" << setw(2) << setfill('0') << seconds;
    return oss.str();
}

// =============================================================================
// SECTION 1: CATALOG.CPP -- Song array add/display/sort/search
// (function bodies unchanged from catalog.cpp, aside from operating on
// the shared Song struct's nested release.albumName/release.releaseYear
// instead of flat album/year fields)
// =============================================================================

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
         << setw(15) << "Genre"
         << setw(10) << "Year"
         << setw(10) << "Duration" << endl;

    cout << setfill('-') << setw(115) << "-" << setfill(' ') << endl;

    for (int i = 0; i < count; i++) {
        cout << left << setw(30) << songs[i].title
             << setw(25) << songs[i].artist
             << setw(25) << songs[i].release.albumName
             << setw(15) << songs[i].genre
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

// ----- NEW: manual Selection Sort by Title + manual Binary Search by Title -----
// The rubric requires Binary Search over data that has been sorted first,
// so this sort exists specifically to guarantee that precondition.

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

// Requires songs[0..count-1] to already be sorted ascending by title.
// The menu handler enforces this by always sorting immediately before
// calling this function.
void binarySearchByTitle(Song songs[], int count) {
    if (count == 0) {
        cout << "\nCatalog is empty. Cannot search." << endl;
        return;
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

// =============================================================================
// SECTION 2: TREE.CPP -- Recently-played stack and Genre BST
// (class bodies unchanged from tree.cpp, aside from:
//   - using the shared Song struct instead of tree.cpp's own Song/AlbumInfo
//   - TreeNode renamed to GenreTreeNode to avoid clashing with AlbumTreeNode)
// =============================================================================

// ----- Stack Component (Recently Played History) -----
class PlayedHistoryStack {
private:
    Song history[STACK_MAX];
    int topIndex;

public:
    PlayedHistoryStack() {
        topIndex = -1; // Stack starts empty
    }

    bool isFull() {
        return topIndex == STACK_MAX - 1;
    }

    bool isEmpty() {
        return topIndex == -1;
    }

    // Push a song onto the history stack when played
    void pushPlayed(Song playedSong) {
        if (isFull()) {
            cout << "History stack overflow! Cannot add more songs.\n";
            return;
        }
        topIndex++;
        history[topIndex] = playedSong;
    }

    // Pop the last played song (Undo / Skip back)
    Song popPlayed() {
        if (isEmpty()) {
            cout << "History is empty!\n";
            // Returns an empty/dummy song if nothing is there
            return Song{"", "", {"", 0}, "", 0};
        }
        Song lastPlayed = history[topIndex];
        topIndex--;
        return lastPlayed;
    }

    // Optional helper to display the current history stack
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

// ----- BST Component (Genre/Artist Hierarchy) -----
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

    // Private helper for recursive insertion
    GenreTreeNode* insertHelper(GenreTreeNode* node, Song song) {
        // If the spot is empty, create the node here
        if (node == nullptr) {
            return new GenreTreeNode(song);
        }

        // Compare genres alphabetically to decide left or right branch
        if (song.genre < node->songData.genre) {
            node->left = insertHelper(node->left, song);
        } else {
            // Equal genres can just go to the right branch
            node->right = insertHelper(node->right, song);
        }
        return node;
    }

    // Private helper for recursive In-Order Traversal (A-Z)
    void inOrderHelper(GenreTreeNode* node) {
        if (node == nullptr) return;

        inOrderHelper(node->left);   // Visit Left

        // Print the current node data
        cout << "[" << node->songData.genre << "] "
             << node->songData.title << " - "
             << node->songData.artist << "\n";

        inOrderHelper(node->right);  // Visit Right
    }

public:
    GenreTree() {
        root = nullptr;
    }

    // Public function to insert a song into the tree
    void insertGenre(Song song) {
        root = insertHelper(root, song);
    }

    // Public function to display the entire tree alphabetically by genre
    void inOrderDisplay() {
        if (root == nullptr) {
            cout << "The genre tree is empty.\n";
            return;
        }
        cout << "--- Songs Sorted Alphabetically by Genre ---\n";
        inOrderHelper(root);
    }
};

// =============================================================================
// SECTION 3: MAIN.CPP -- Album BST + File Save/Load
// (function bodies unchanged from main.cpp, aside from TreeNode ->
// AlbumTreeNode rename, and Song's album/year now going through
// song.release.albumName / song.release.releaseYear)
// =============================================================================

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

// In-order display of the album tree (added so albums are actually
// viewable from the menu; same traversal pattern as GenreTree above)
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

// ----- NEW: Catalog Array Persistence -----
// The Album tree above already saves/loads fine, but that only covers
// songs added through "Add Album". Songs added through "Add Song" live
// in the raw catalog[] array and were never being saved. These two
// functions give that array its own small ledger file so it persists
// too, satisfying the rubric's "re-populate the program's runtime
// memory storage array" requirement.

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

// ----- NEW: Starter Catalog (Top 5 Songs of Quarantine) -----
// Pre-loads the catalog with well-known 2020 quarantine-era hits so the
// system isn't empty on first run. Only runs if the catalog is empty
// (i.e. no save file existed yet), so it won't duplicate entries on
// every subsequent launch.
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

// ----- NEW: Starter Album (Starboy by The Weeknd) -----
// Pre-loads the Album BST with The Weeknd's "Starboy" (2016) so there's
// an existing album to browse/search on first run. Only inserted if the
// album tree came back empty from loadFromFile (i.e. no songs.txt yet),
// so it won't be re-added every time the program launches.
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

// Lets the user build an Album (with its own tracklist) and insert it
// into the Album BST. Reuses the same input style as addSong() above.
void addAlbum(AlbumTreeNode*& musicCatalog) {
    Album newAlbum;

    cout << "\n--- Add a New Album ---" << endl;
    cout << "Enter Album Name: ";
    getline(cin, newAlbum.albumName);

    cout << "Enter Album Artist: ";
    getline(cin, newAlbum.albumArtist);

    cout << "Enter Release Year: ";
    cin >> newAlbum.releaseYear;

    int trackCount;
    cout << "How many tracks on this album? ";
    cin >> trackCount;
    cin.ignore();

    for (int i = 0; i < trackCount; i++) {
        Song s;
        cout << "\nTrack " << (i + 1) << ":\n";
        cout << "  Title: ";
        getline(cin, s.title);
        s.artist = newAlbum.albumArtist;
        s.release.albumName = newAlbum.albumName;
        cout << "  Genre: ";
        getline(cin, s.genre);
        s.release.releaseYear = newAlbum.releaseYear;
        cout << "  Duration (seconds): ";
        cin >> s.duration;
        cin.ignore();
        newAlbum.tracklist.push_back(s);
    }

    musicCatalog = insertAlbum(musicCatalog, newAlbum);
    cout << "\nAlbum added successfully!" << endl;
}

// =============================================================================
// COMBINED MENU  --  boot sequence, logo, and boxed layout
// =============================================================================

const int BOX_WIDTH = 57; // interior width of the menu box, between the '|' chars

// Centers text inside a field of the given width (used for the logo/title).
string centerText(const string& text, int width) {
    if ((int)text.size() >= width) return text.substr(0, width);
    int left = (width - (int)text.size()) / 2;
    int right = width - (int)text.size() - left;
    return string(left, ' ') + text + string(right, ' ');
}

// Prints one line of the box: "| <padded text> |"
void printBoxLine(const string& text) {
    cout << "|" << left << setw(BOX_WIDTH) << text << "|" << endl;
}

// Prints a divider like "+----------------------------------------+"
void printBoxDivider(char fill = '-') {
    cout << "+" << string(BOX_WIDTH, fill) << "+" << endl;
}

// Builds "  [1] Something          [2] Something Else" padded into two columns.
string twoCol(const string& a, const string& b) {
    ostringstream oss;
    oss << "  " << left << setw(27) << a << " " << b;
    return oss.str();
}

// ---- Fake boot sequence, shown once when the program starts ----
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

// ---- ASCII logo, shown once after the boot sequence ----
const int LOGO_WIDTH = 50; // interior width of the record-sleeve box below

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

// ----- NEW: Play a Song flow -----
// Shows the available songs, plays whichever one is typed, then asks
// whether to play another or head back to the main menu -- instead of
// dumping the user straight back to the main menu after one play.
void playSongMenu(Song catalog[], int songCount, PlayedHistoryStack &historyStack) {
    if (songCount == 0) {
        cout << "\nCatalog is empty. Add a song first." << endl;
        return;
    }

    bool keepPlaying = true;

    while (keepPlaying) {
        cout << "\n--- Available Songs ---" << endl;
        for (int i = 0; i < songCount; i++) {
            cout << "  " << catalog[i].title
                 << " - " << catalog[i].artist << endl;
        }

        string playTitle;
        cout << "\nEnter the title of the song to play: ";
        getline(cin, playTitle);

        bool played = false;
        for (int i = 0; i < songCount; i++) {
            if (catalog[i].title == playTitle) {
                historyStack.pushPlayed(catalog[i]);
                cout << "Now playing: " << catalog[i].title
                     << " by " << catalog[i].artist << endl;
                played = true;
                break;
            }
        }
        if (!played) {
            cout << "Song not found in catalog." << endl;
        }

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

    // First-ever run (no saved catalog yet) -> load starter songs
    if (songCount == 0) {
        seedTopQuarantineSongs(catalog, songCount);
    }

    PlayedHistoryStack historyStack;
    GenreTree genreTree;

    // Rebuild the genre tree from any songs reloaded from disk, so genre
    // browsing reflects songs from previous sessions too.
    for (int i = 0; i < songCount; i++) {
        genreTree.insertGenre(catalog[i]);
    }

    AlbumTreeNode* musicCatalog = nullptr;
    string saveFileName = "songs.txt";
    loadFromFile(musicCatalog, saveFileName);

    // First-ever run (no saved albums yet) -> load starter album
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
                addSong(catalog, songCount);
                // keep the genre tree in sync with the flat catalog
                if (songCount > 0) {
                    genreTree.insertGenre(catalog[songCount - 1]);
                }
                break;
            }
            case 2:
                displayAllSongs(catalog, songCount);
                break;
            case 3:
                selectionSortByDuration(catalog, songCount);
                break;
            case 4:
                searchSongByTitle(catalog, songCount);
                break;
            case 5: {
                cout << "\n(Sorting catalog alphabetically by title first, "
                        "as required for binary search...)" << endl;
                selectionSortByTitle(catalog, songCount);
                binarySearchByTitle(catalog, songCount);
                break;
            }
            case 6: {
                playSongMenu(catalog, songCount, historyStack);
                break;
            }
            case 7:
                historyStack.displayRecentlyPlayed();
                break;
            case 8: {
                Song skipped = historyStack.popPlayed();
                if (!skipped.title.empty()) {
                    cout << "Skipped back from: " << skipped.title << endl;
                }
                break;
            }
            case 9:
                genreTree.inOrderDisplay();
                break;
            case 10:
                addAlbum(musicCatalog);
                break;
            case 11:
                displayAlbums(musicCatalog);
                break;
            case 12:
                cout << "Saving and exiting the program. Goodbye!" << endl;
                break;
            default:
                cout << "Invalid choice. Please enter a number between 1 and 12." << endl;
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
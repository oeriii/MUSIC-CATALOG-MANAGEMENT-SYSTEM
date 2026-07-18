// Here we will do our menu loop and functions like loadFromFile() / saveToFile()

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept> 
#include <algorithm> 

using namespace std;

const int MAX_LIMIT = 100;

// Structures (Song, Album, and Treenode)

// Structure for Song from catalog.cpp
// ... Can be replaced when all files are merged
struct Song {
    string title;
    string artist;
    string album;
    string genre;
    int year;
    int duration;
};

// Structure for Album 
// ... Information per Song
struct Album {
    string albumName;
    string albumArtist;
    int releaseYear;
    vector<Song> tracklist;
};

// Structure for TreeNode
struct TreeNode {
    Album data;
    TreeNode* left;
    TreeNode* right;
};

// Converts string to lowercase
string toLower(string str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// Tree Helper Functions

TreeNode* insertAlbum(TreeNode* node, const Album& newAlbum) {
    if (node == nullptr) {
        TreeNode* newNode = new TreeNode;
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

void clearTree(TreeNode*& node) { 
    if (node == nullptr) return;
    clearTree(node->left);
    clearTree(node->right);
    delete node;
    node = nullptr; 
}

// File Handling Functions 

void saveSong(ofstream& outFile, const Song& song) {
    outFile << song.title << "|"
            << song.artist << "|"
            << song.album << "|"
            << song.genre << "|"
            << song.year << "|"
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

void saveToFile(TreeNode* node, ofstream& outFile) {
    if (node == nullptr) return;
    saveAlbum(outFile, node->data);
    saveToFile(node->left, outFile);
    saveToFile(node->right, outFile);
}

void loadFromFile(TreeNode*& musicCatalog, const string& fileName) {
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
                string songYearStr, durationStr;

                if (!getline(songSS, tempSong.title, '|') ||
                    !getline(songSS, tempSong.artist, '|') ||
                    !getline(songSS, tempSong.album, '|') ||
                    !getline(songSS, tempSong.genre, '|') ||
                    !getline(songSS, songYearStr, '|') ||
                    !getline(songSS, durationStr, '|')) {
                    albumCorrupted = true; 
                    break; 
                }

                tempSong.year = stoi(songYearStr);
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

int main() {
    TreeNode* musicCatalog = nullptr;
    string saveFileName = "songs.txt";


    loadFromFile(musicCatalog, saveFileName);

    // MENU LOOP GOES HERE 
    
    ofstream outFile(saveFileName);
    if (outFile.is_open()) {
        saveToFile(musicCatalog, outFile);
        outFile.close();
        cout << "Catalog changes committed safely to disk.\n";
    }

    clearTree(musicCatalog);
    return 0;
}

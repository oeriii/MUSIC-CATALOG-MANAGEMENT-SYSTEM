#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

const int MAX_SONGS = 100;

struct Song {
    string title;
    string artist;
    string album;
    string genre;
    int year;
    int duration;
};

void addSong(Song songs[], int &count);
void displayAllSongs(Song songs[], int count);
void selectionSortByDuration(Song songs[], int count);
void searchSongByTitle(Song songs[], int count);

int main() {
    Song catalog[MAX_SONGS];
    int songCount = 0;
    int choice;

    do {
        cout << "\n===================================" << endl;
        cout << "  Music Catalog Management System" << endl;
        cout << "===================================" << endl;
        cout << "1. Add Song" << endl;
        cout << "2. Display All Songs" << endl;
        cout << "3. Sort Songs by Duration" << endl;
        cout << "4. Search Song by Title" << endl;
        cout << "5. Exit" << endl;
        cout << "Enter your choice (1-5): ";
        cin >> choice;

        switch (choice) {
            case 1:
                addSong(catalog, songCount);
                break;
            case 2:
                displayAllSongs(catalog, songCount);
                break;
            case 3:
                selectionSortByDuration(catalog, songCount);
                break;
            case 4:
                searchSongByTitle(catalog, songCount);
                break;
            case 5:
                cout << "Exiting the program. Goodbye!" << endl;
                break;
            default:
                cout << "Invalid choice. Please enter a number between 1 and 5." << endl;
        }
    } while (choice != 5);

    return 0;
}

void addSong(Song songs[], int &count) {
    if (count >= MAX_SONGS) {
        cout << "Error: The catalog is full (Max " << MAX_SONGS << " songs)." << endl;
        return;
    }

    cin.ignore();

    cout << "\n--- Add a New Song ---" << endl;
    cout << "Enter Title: ";
    getline(cin, songs[count].title);

    cout << "Enter Artist: ";
    getline(cin, songs[count].artist);

    cout << "Enter Album: ";
    getline(cin, songs[count].album);

    cout << "Enter Genre: ";
    getline(cin, songs[count].genre);

    cout << "Enter Year: ";
    cin >> songs[count].year;

    cout << "Enter Duration (in seconds): ";
    cin >> songs[count].duration;

    count++;

    cout << "Song added successfully!" << endl;
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
         << setw(10) << "Duration(s)" << endl;
         
    cout << setfill('-') << setw(115) << "-" << setfill(' ') << endl;

    for (int i = 0; i < count; i++) {
        cout << left << setw(30) << songs[i].title 
             << setw(25) << songs[i].artist 
             << setw(25) << songs[i].album 
             << setw(15) << songs[i].genre 
             << setw(10) << songs[i].year 
             << setw(10) << songs[i].duration << endl;
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

    cin.ignore();
    string searchTitle;
    cout << "\nEnter Song Title to search: ";
    getline(cin, searchTitle);

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (songs[i].title == searchTitle) {
            cout << "\n--- Song Found ---" << endl;
            cout << "Title:    " << songs[i].title << endl;
            cout << "Artist:   " << songs[i].artist << endl;
            cout << "Album:    " << songs[i].album << endl;
            cout << "Genre:    " << songs[i].genre << endl;
            cout << "Year:     " << songs[i].year << endl;
            cout << "Duration: " << songs[i].duration << " seconds" << endl;
            
            found = true;
            break;
        }
    }

    if (!found) {
        cout << "\nSong not found." << endl;
    }
}


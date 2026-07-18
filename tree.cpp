// here we do stack functions (pushPlayed, popPlayed, displayRecentlyPlayed) and BST functions (insertGenre, inOrderDisplay)

#include <iostream>
#include <string>

// ============================================================================
// 1. TEMPORARY DUMMY STRUCTURES
// (Replace these once Catalog.cpp / Main.cpp share the official ones)
// ============================================================================
struct AlbumInfo {
    std::string albumName;
    int year;
};

struct Song {
    int trackID;
    std::string title;
    std::string artist;
    std::string genre;
    AlbumInfo album;
};

// ============================================================================
// 2. STACK COMPONENT (Recently Played History)
// ============================================================================
const int STACK_MAX = 50; // Required based on the rubric rules

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
            std::cout << "History stack overflow! Cannot add more songs.\n";
            return;
        }
        topIndex++;
        history[topIndex] = playedSong;
    }
    
    // Pop the last played song (Undo / Skip back)
    Song popPlayed() {
        if (isEmpty()) {
            std::cout << "History is empty!\n";
            // Returns an empty/dummy song if nothing is there
            return Song{-1, "", "", "", {"", 0}}; 
        }
        Song lastPlayed = history[topIndex];
        topIndex--;
        return lastPlayed;
    }

    // displayHistory
    // Optional helper to display the current history stack
    void displayRecentlyPlayed() {
        if (isEmpty()) {
            std::cout << "No recently played songs.\n";
            return;
        }
        std::cout << "--- Recently Played History (Latest First) ---\n";
        for (int i = topIndex; i >= 0; i--) {
            std::cout << history[i].title << " by " << history[i].artist << "\n";
        }
    }
};

// ============================================================================
// 3. BST COMPONENT (Genre/Artist Hierarchy)
// ============================================================================
struct TreeNode {
    Song songData;
    TreeNode* left;
    TreeNode* right;

    TreeNode(Song song) {
        songData = song;
        left = nullptr;
        right = nullptr;
    }
};

class GenreTree {
private:
    TreeNode* root;

    // Private helper for recursive insertion
    TreeNode* insertHelper(TreeNode* node, Song song) {
        // If the spot is empty, create the node here
        if (node == nullptr) {
            return new TreeNode(song);
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
    void inOrderHelper(TreeNode* node) {
        if (node == nullptr) return;

        inOrderHelper(node->left);   // Visit Left
        
        // Print the current node data
        std::cout << "[" << node->songData.genre << "] " 
                  << node->songData.title << " - " 
                  << node->songData.artist << "\n";
                  
        inOrderHelper(node->right);  // Visit Right
    }

public:
    GenreTree() {
        root = nullptr;
    }

    // insert(Song song)
    // Public function to insert a song into the tree
    void insertGenre(Song song) {
        root = insertHelper(root, song);
    }

    // displayAlphabetically()
    // Public function to display the entire tree alphabetically by genre
    void inOrderDisplay() {
        if (root == nullptr) {
            std::cout << "The genre tree is empty.\n";
            return;
        }
        std::cout << "--- Songs Sorted Alphabetically by Genre ---\n";
        inOrderHelper(root);
    }
};

// ============================================================================
// 4. TEST BENCH (You can delete or comment this main out during integration)
// ============================================================================
int main() {
    // Create dummy songs to test your logic
    Song s1 = {1, "Blinding Lights", "The Weeknd", "Pop", {"After Hours", 2020}};
    Song s2 = {2, "Bohemian Rhapsody", "Queen", "Rock", {"A Night at the Opera", 1975}};
    Song s3 = {3, "Bad Guy", "Billie Eilish", "Pop", {"When We All Fall Asleep", 2019}};
    Song s4 = {4, "Enter Sandman", "Metallica", "Metal", {"Metallica", 1991}};

    // --- Testing the Stack ---
    PlayedHistoryStack historyStack;
    std::cout << "Playing songs...\n";
    historyStack.pushPlayed(s1);
    historyStack.pushPlayed(s2);
    historyStack.displayRecentlyPlayed();

    std::cout << "\nPopping (Skipping back):\n";
    Song skipped = historyStack.popPlayed();
    std::cout << "Skipped back from: " << skipped.title << "\n\n";
    historyStack.displayRecentlyPlayed();

    std::cout << "\n============================================\n\n";

    // --- Testing the BST ---
    GenreTree structureTree;
    structureTree.insertGenre(s1);
    structureTree.insertGenre(s2);
    structureTree.insertGenre(s3);
    structureTree.insertGenre(s4);

    // Should output: Metal -> Pop -> Pop -> Rock
    structureTree.inOrderDisplay();

    return 0;
}

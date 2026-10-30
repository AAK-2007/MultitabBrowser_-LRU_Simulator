/*
 * =====================================================================================
 *  Multi-Tab Browser & LRU Cache Simulator
 * =====================================================================================
 *  Course      : PCCST303 - Data Structures and Algorithms
 *  Description : A command-line simulation of a multi-tab web browser.
 *
 *  Data Structures used :
 *      1. Doubly Linked List  -> manages the list of open browser tabs
 *                                 (open / close / switch between tabs)
 *      2. Stack (per tab)     -> Back Stack + Forward Stack for page navigation
 *      3. Hash Map + Doubly
 *         Linked List         -> LRU Cache for recently visited pages
 *
 *  Design summary:
 *      - TabList  : a doubly linked list where every node is a "Tab".
 *      - Tab      : holds its own Back/Forward stacks and current URL.
 *      - LRUCache : fixed-capacity cache shared across all tabs, backed by
 *                   an unordered_map.
 *
 * =====================================================================================
 */

#include <iostream>
#include <string>
#include <unordered_map>
#include <stack>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace std;

/* =====================================================================================
 *  SECTION 1 : LRU CACHE  (Hash Map + Doubly Linked List)
 * =====================================================================================
 *  Stores the most recently visited pages across the whole browser.
 *  - Doubly linked list keeps pages ordered from Most Recently Used (head)
 *    to Least Recently Used (tail).
 *  - unordered_map<url, node*> gives access to any node so it can be
 *    unlinked and moved to the head without scanning the list.
 * =====================================================================================
 */

class LRUCache {
private:
    // Node of the internal doubly linked list; stores one cached page.
    struct CacheNode {
        string url;
        CacheNode* prev;
        CacheNode* next;
        CacheNode(const string& u) : url(u), prev(nullptr), next(nullptr) {}
    };

    int capacity;                                   // Max number of pages the cache can hold
    CacheNode* head;                                 // Dummy head (MRU side)
    CacheNode* tail;                                 // Dummy tail (LRU side)
    unordered_map<string, CacheNode*> lookup;        // url -> node, for O(1) access

    // Unlink a node from wherever it currently sits in the list. O(1).
    void detach(CacheNode* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    // Insert a node right after the dummy head (i.e. mark it Most Recently Used). O(1).
    void attachToFront(CacheNode* node) {
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }

public:
    explicit LRUCache(int cap) : capacity(cap) {
        head = new CacheNode("HEAD");
        tail = new CacheNode("TAIL");
        head->next = tail;
        tail->prev = head;
    }

    ~LRUCache() {
        CacheNode* curr = head;
        while (curr) {
            CacheNode* nxt = curr->next;
            delete curr;
            curr = nxt;
        }
    }

    // Access a page. If present, promote it to MRU and return true (cache hit).
    // If absent, return false (cache miss) -- caller decides whether to insert it.
    bool access(const string& url) {
        auto it = lookup.find(url);
        if (it == lookup.end()) {
            return false; // cache miss
        }
        CacheNode* node = it->second;
        detach(node);
        attachToFront(node);
        return true; // cache hit
    }

    // Insert a newly visited page into the cache (called after a miss).
    // Evicts the Least Recently Used page automatically if at capacity.
    void put(const string& url) {
        if (lookup.find(url) != lookup.end()) {
            // Already cached -- just refresh its position.
            access(url);
            return;
        }

        if (static_cast<int>(lookup.size()) >= capacity) {
            // Evict the LRU entry (the node right before the dummy tail).
            CacheNode* lru = tail->prev;
            if (lru != head) {
                cout << "  [Cache] Capacity full. Evicting LRU page: " << lru->url << "\n";
                detach(lru);
                lookup.erase(lru->url);
                delete lru;
            }
        }

        CacheNode* fresh = new CacheNode(url);
        attachToFront(fresh);
        lookup[url] = fresh;
    }

    // Print cache contents from MRU to LRU.
    void display() const {
        if (lookup.empty()) {
            cout << "  Cache is empty.\n";
            return;
        }
        cout << "  Cache (MRU -> LRU): ";
        CacheNode* curr = head->next;
        bool first = true;
        while (curr != tail) {
            if (!first) cout << " -> ";
            cout << curr->url;
            first = false;
            curr = curr->next;
        }
        cout << "\n";
    }
};

/* =====================================================================================
 *  SECTION 2 : TAB  (holds per-tab navigation history via two Stacks)
 * =====================================================================================
 *  Back Stack    : pages visited before the current one (top = most recent previous page)
 *  Forward Stack : pages you navigated back from (cleared when a new page is visited)
 * =====================================================================================
 */

class Tab {
public:
    int id;                       // Unique tab identifier
    string currentUrl;            // Page currently displayed in this tab
    stack<string> backStack;      // History available via "Back"
    stack<string> forwardStack;   // History available via "Forward"

    Tab(int tabId, const string& startUrl)
        : id(tabId), currentUrl(startUrl) {}

    // Visit a new page: push current page to Back stack, clear Forward stack.
    void visit(const string& url) {
        if (!currentUrl.empty()) {
            backStack.push(currentUrl);
        }
        currentUrl = url;
        // A fresh navigation invalidates any old "forward" history.
        while (!forwardStack.empty()) forwardStack.pop();
    }

    // Navigate back one page. Returns false if there is nothing to go back to.
    bool goBack() {
        if (backStack.empty()) return false;
        forwardStack.push(currentUrl);
        currentUrl = backStack.top();
        backStack.pop();
        return true;
    }

    // Navigate forward one page. Returns false if there is nothing to go forward to.
    bool goForward() {
        if (forwardStack.empty()) return false;
        backStack.push(currentUrl);
        currentUrl = forwardStack.top();
        forwardStack.pop();
        return true;
    }
};

/* =====================================================================================
 *  SECTION 3 : TAB LIST  (Doubly Linked List of open Tabs)
 * =====================================================================================
 *  Manages the collection of open tabs: open, close, switch (next/previous).
 * =====================================================================================
 */

class TabList {
private:
    // Doubly linked list node wrapping a Tab.
    struct TabNode {
        Tab tab;
        TabNode* prev;
        TabNode* next;
        TabNode(int id, const string& url) : tab(id, url), prev(nullptr), next(nullptr) {}
    };

    TabNode* head;          // First open tab
    TabNode* tail;          // Last open tab
    TabNode* active;        // Currently active tab
    int tabCount;
    int nextTabId;          // Auto-incrementing id for new tabs

    // Find a tab node by id; returns nullptr if not found. O(n) but n = open tabs (small).
    TabNode* findNode(int id) const {
        TabNode* curr = head;
        while (curr) {
            if (curr->tab.id == id) return curr;
            curr = curr->next;
        }
        return nullptr;
    }

public:
    TabList() : head(nullptr), tail(nullptr), active(nullptr), tabCount(0), nextTabId(1) {}

    ~TabList() {
        TabNode* curr = head;
        while (curr) {
            TabNode* nxt = curr->next;
            delete curr;
            curr = nxt;
        }
    }

    int getTabCount() const { return tabCount; }
    bool hasActiveTab() const { return active != nullptr; }
    Tab* getActiveTab() const { return active ? &active->tab : nullptr; }

    // Open a new tab at the end of the list and make it active. O(1).
    Tab* openTab(const string& startUrl = "about:blank") {
        TabNode* node = new TabNode(nextTabId++, startUrl);
        if (!head) {
            head = tail = node;
        }
        else {
            tail->next = node;
            node->prev = tail;
            tail = node;
        }
        active = node;
        tabCount++;
        cout << "  Opened Tab #" << node->tab.id << " (" << startUrl << ")\n";
        return &node->tab;
    }

    // Close a tab by id. If it was active, activate a sensible neighbor. O(1) once found.
    bool closeTab(int id) {
        TabNode* node = findNode(id);
        if (!node) {
            cout << "  Tab #" << id << " not found.\n";
            return false;
        }

        TabNode* newActiveCandidate = nullptr;
        if (node == active) {
            // Prefer the next tab, else the previous, else none.
            newActiveCandidate = node->next ? node->next : node->prev;
        }

        // Unlink node from the list.
        if (node->prev) node->prev->next = node->next;
        else head = node->next;

        if (node->next) node->next->prev = node->prev;
        else tail = node->prev;

        if (node == active) {
            active = newActiveCandidate;
        }

        cout << "  Closed Tab #" << id << "\n";
        delete node;
        tabCount--;
        return true;
    }

    // Switch active tab to the given id. O(n) to locate, O(1) to switch.
    bool switchTo(int id) {
        TabNode* node = findNode(id);
        if (!node) {
            cout << "  Tab #" << id << " not found.\n";
            return false;
        }
        active = node;
        cout << "  Switched to Tab #" << id << "\n";
        return true;
    }

    // Move active tab pointer to the next tab in the list (wraps around).
    bool nextTab() {
        if (!active) return false;
        active = active->next ? active->next : head;
        return true;
    }

    // Move active tab pointer to the previous tab in the list (wraps around).
    bool prevTab() {
        if (!active) return false;
        active = active->prev ? active->prev : tail;
        return true;
    }

    // Display all open tabs, marking the active one.
    void display() const {
        if (!head) {
            cout << "  No tabs open.\n";
            return;
        }
        cout << "  Open Tabs:\n";
        TabNode* curr = head;
        while (curr) {
            cout << "    " << (curr == active ? "-> " : "   ")
                << "Tab #" << curr->tab.id << " : " << curr->tab.currentUrl << "\n";
            curr = curr->next;
        }
    }
};

/* =====================================================================================
 *  SECTION 4 : BROWSER  (ties TabList + LRUCache together)
 * =====================================================================================
 */

class Browser {
private:
    TabList tabs;
    LRUCache cache;

public:
    explicit Browser(int cacheCapacity) : cache(cacheCapacity) {}

    void openTab() {
        tabs.openTab();
    }

    void closeTab(int id) {
        tabs.closeTab(id);
    }

    void switchTab(int id) {
        tabs.switchTo(id);
    }

    void nextTab() {
        if (!tabs.nextTab()) cout << "  No tabs open.\n";
    }

    void prevTab() {
        if (!tabs.prevTab()) cout << "  No tabs open.\n";
    }

    // Visit a page in the active tab; also updates the shared LRU cache.
    void visitPage(const string& url) {
        Tab* active = tabs.getActiveTab();
        if (!active) {
            cout << "  No active tab. Please open a tab first.\n";
            return;
        }
        active->visit(url);
        cout << "  Tab #" << active->id << " navigated to: " << url << "\n";

        if (cache.access(url)) {
            cout << "  Found in cache ! '" << url << "' was already cached (moved to MRU).\n";
        }
        else {
            cout << "  Not Found in cache. Adding '" << url << "' to cache.\n";
            cache.put(url);
        }
    }

    void goBack() {
        Tab* active = tabs.getActiveTab();
        if (!active) { cout << "  No active tab.\n"; return; }
        if (active->goBack()) {
            cout << "  Tab #" << active->id << " went back to: " << active->currentUrl << "\n";
            cache.access(active->currentUrl); // Revisiting counts as an access.
        }
        else {
            cout << "  No previous page in history for this tab.\n";
        }
    }

    void goForward() {
        Tab* active = tabs.getActiveTab();
        if (!active) { cout << "  No active tab.\n"; return; }
        if (active->goForward()) {
            cout << "  Tab #" << active->id << " went forward to: " << active->currentUrl << "\n";
            cache.access(active->currentUrl);
        }
        else {
            cout << "  No forward page in history for this tab.\n";
        }
    }

    void showTabs() const {
        tabs.display();
    }

    void showCache() const {
        cache.display();
    }
};

/* =====================================================================================
 *  SECTION 5 : MENU-DRIVEN CLI
 * =====================================================================================
 */

void printMenu() {
    cout << "\n==================== MULTI-TAB BROWSER SIMULATOR ======================\n"
        << "\n"
        << " 1. Open new tab    2. Close tab        3. Switch to tab (by id)\n"
        << " 4. Next tab        5. Previous tab     6. Visit a page (in active tab)\n"
        << " 7. Go Back         8. Go Forward       9. Show all open tabs\n"
        << "10. Show LRU cache contents\n"
        << "\n"
        << " 0. Exit\n"
        << "\n"
        << "=======================================================================\n"
        << "Enter choice: ";
}

// Ask for the LRU cache capacity until a positive whole number is entered.
// The whole line is read and checked, so zero, negative numbers, text and
// mixed input such as "5abc" or "3.5" are all rejected and the prompt is shown again.
// Returns -1 only if the input ends (Ctrl+Z on Windows, Ctrl+D on Linux/macOS).

int readCacheCapacity() {
    string line;
    while (true) {
        cout << "Enter LRU cache capacity (e.g. 3): ";
        if (!getline(cin, line)) {
            return -1;                        // end of input, nothing more to read
        }
        istringstream in(line);
        int value;
        char extra;
        if ((in >> value) && !(in >> extra) && value > 0) {
            return value;                     // valid: a single positive integer
        }
        cout << "  Invalid capacity. Please enter a positive whole number.\n";
    }
}

int main() {
    int cacheCapacity = readCacheCapacity();
    if (cacheCapacity <= 0) {
        cout << "\n  No capacity entered. Exiting simulator.\n";
        return 0;
    }

    Browser browser(cacheCapacity);
    int choice;

    do {
        printMenu();
        if (!(cin >> choice)) {
            // Recover from invalid (non-numeric) input.
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  Invalid input. Please enter a number.\n";
            choice = -1; // keep main loop running
            continue;
        }

        switch (choice) {
        case 1:
            browser.openTab();
            break;
        case 2: {
            int id;
            cout << "  Enter tab id to close: ";
            cin >> id;
            browser.closeTab(id);
            break;
        }
        case 3: {
            int id;
            cout << "  Enter tab id to switch to: ";
            cin >> id;
            browser.switchTab(id);
            break;
        }
        case 4:
            browser.nextTab();
            break;
        case 5:
            browser.prevTab();
            break;
        case 6: {
            string url;
            cout << "  Enter URL to visit: ";
            cin >> url;
            browser.visitPage(url);
            break;
        }
        case 7:
            browser.goBack();
            break;
        case 8:
            browser.goForward();
            break;
        case 9:
            browser.showTabs();
            break;
        case 10:
            browser.showCache();
            break;
        case 0:
            cout << "  Exiting simulator. Goodbye!\n";
            break;
        default:
            cout << "  Invalid choice. Try again.\n";
        }
    } while (choice != 0);

    return 0;
}
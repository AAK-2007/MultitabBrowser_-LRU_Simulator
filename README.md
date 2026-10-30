# Multi-Tab Browser & LRU Cache Simulator

A command-line simulation of a multi-tab web browser, written in C++ for
**PCCST303 – Data Structures and Algorithms**.

It models three things a real browser has to do — keep several tabs open,
remember where each tab has been, and cache recently visited pages — and each
one is built on a different data structure.

---

## Data structures used

| Feature | Data structure | Why |
|---|---|---|
| Open tabs (open / close / switch) | **Doubly linked list** | tabs must be traversed in both directions and removed from the middle in O(1) |
| Per-tab navigation history | **Two stacks** (back + forward) | LIFO matches how Back and Forward behave |
| Recently visited pages | **Hash map + doubly linked list** | the map gives O(1) lookup, the list keeps pages ordered MRU → LRU |

### Why the LRU cache needs both structures

A doubly linked list alone can keep pages in order of use, but finding a given
page means scanning the list — O(n). A hash map alone gives instant lookup but
has no notion of order. Combining them gives **O(1) for every operation**: the
map finds the node, and the node's `prev`/`next` pointers let it be unlinked and
moved to the front without touching anything else.

---

## Operations

| Menu | Operation | Complexity |
|---|---|---|
| 1 | Open a new tab | O(1) |
| 2 | Close a tab by id | O(n) to find, O(1) to unlink |
| 3 | Switch to a tab by id | O(n) |
| 4 / 5 | Next / previous tab (wraps around) | O(1) |
| 6 | Visit a page in the active tab | O(1) |
| 7 / 8 | Go back / forward | O(1) |
| 9 | List all open tabs | O(n) |
| 10 | Show the cache from MRU to LRU | O(capacity) |

Visiting a new page clears that tab's forward history, exactly as a real
browser does.

---

## Build and run

```bash
g++ MultitabBrowser__LRU_Simulator.cpp -o browser
./browser
```

Or with the included makefile:

```bash
make
./browser
```

On Windows with MinGW:

```bash
g++ MultitabBrowser__LRU_Simulator.cpp -o browser.exe
browser.exe
```

---

## Sample session

```
Enter LRU cache capacity (e.g. 3): 3

1  -> open a tab
6  -> visit a.com
6  -> visit b.com
6  -> visit c.com
6  -> visit d.com      [Cache] Capacity full. Evicting LRU page: a.com
7  -> go back to c.com
10 -> Cache (MRU -> LRU): c.com -> d.com -> b.com
```

The cache holds three pages. Visiting a fourth evicts the least recently used
one, and going back to `c.com` counts as an access, promoting it to the front.

---

## Project structure

```
.
├── MultitabBrowser__LRU_Simulator.cpp   all source, organised in 5 sections
├── Makefile
├── .gitignore
└── README.md
```

The source is divided into: LRU cache, Tab, TabList, Browser, and the
menu-driven CLI.

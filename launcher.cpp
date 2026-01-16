#include <ncurses.h>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>
#include <deque>
#include <clocale>
#include <cctype>

namespace fs = std::filesystem;

struct Game {
    std::string filename;
    std::string displayName;
    std::string description;
    bool isFavorite = false;
};

std::vector<Game> allGames;
std::vector<Game*> visibleGames;
std::deque<std::string> recents;
std::set<std::string> favorites;

int selectedIdx = 0;
bool viewBlockMode = false;
bool searchMode = false;
std::string searchQuery = "";
int currentTab = 0;

const char* DATA_FILE = "launcher.dat";
const std::string EXEC_DIR = "./exec";

int utf8_visual_length(const std::string& str) {
    int length = 0;
    for (size_t i = 0; i < str.length();) {
        char c = str[i];
        if ((c & 0x80) == 0) { i += 1; length++; } 
        else if ((c & 0xE0) == 0xC0) { i += 2; length++; }
        else if ((c & 0xF0) == 0xE0) { i += 3; length++; } 
        else if ((c & 0xF8) == 0xF0) { i += 4; length++; }
        else i += 1; 
    }
    return length;
}

void enrichGameData(Game& g) {
    if (g.filename == "snake") { 
        g.displayName = "SNAKE"; 
        g.description = "Classic Snake game. Eat apples, don't hit walls."; 
    }
    else if (g.filename == "2048") { 
        g.displayName = "2048"; 
        g.description = "Join the numbers and get to the 2048 tile!"; 
    }
    else if (g.filename == "minesweeper") { 
        g.displayName = "MINESWEEPER"; 
        g.description = "Find mines using logic and flood fill."; 
    }
    else if (g.filename == "tetris") { 
        g.displayName = "TETRIS"; 
        g.description = "Stack blocks and clear lines before it's too late."; 
    }
    else if (g.filename == "sudoku") { 
        g.displayName = "SUDOKU"; 
        g.description = "Logic puzzle. Fill the grid with numbers 1-9."; 
    }
    else if (g.filename == "tic-tac-toe" || g.filename == "tictactoe") {
        g.displayName = "TIC-TAC-TOE";
        g.description = "Classic X's and O's. Get three in a row to win!";
    }
    else if (g.filename == "pacman" || g.filename == "tictactoe") {
        g.displayName = "PAC-MAN";
        g.description = "Navigate the maze, eat pellets, and avoid ghosts!";
    }
    else {
        g.displayName = g.filename;
        std::transform(g.displayName.begin(), g.displayName.end(), g.displayName.begin(), ::toupper);
        g.description = "Unknown game executable.";
    }
}

void saveData() {
    std::ofstream f(DATA_FILE);
    if (!f.is_open()) return;

    f << "FAVORITES" << std::endl;
    for (const auto& fav : favorites) f << fav << std::endl;
    
    f << "RECENTS" << std::endl;
    for (const auto& rec : recents) f << rec << std::endl;
    
    f.close();
}

void loadData() {
    std::ifstream f(DATA_FILE);
    if (!f.is_open()) return;

    std::string line, mode;
    while (std::getline(f, line)) {
        if (line == "FAVORITES") { mode = "FAV"; continue; }
        if (line == "RECENTS") { mode = "REC"; continue; }
        
        if (!line.empty()) {
            if (mode == "FAV") favorites.insert(line);
            if (mode == "REC") recents.push_back(line);
        }
    }
    f.close();
}

void scanGames() {
    allGames.clear();
    if (!fs::exists(EXEC_DIR)) fs::create_directory(EXEC_DIR);

    for (const auto& entry : fs::directory_iterator(EXEC_DIR)) {
        if (entry.is_directory()) continue;

        std::string fname = entry.path().filename().string();
        if (fname[0] == '.') continue;
        if (fname.length() > 2 && fname.substr(fname.length()-2) == ".o") continue;

        Game g;
        g.filename = fname;
        enrichGameData(g);
        allGames.push_back(g);
    }
}

void filterGames() {
    visibleGames.clear();
    for (auto& g : allGames) {
        g.isFavorite = (favorites.find(g.filename) != favorites.end());

        if (currentTab == 1 && !g.isFavorite) continue;
        if (currentTab == 2) {
            bool isRecent = false;
            for(auto& r : recents) if(r == g.filename) isRecent = true;
            if(!isRecent) continue;
        }

        if (!searchQuery.empty()) {
            std::string nameLower = g.displayName;
            std::string queryLower = searchQuery;
            
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), 
                           [](unsigned char c){ return std::tolower(c); });
            std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), 
                           [](unsigned char c){ return std::tolower(c); });
            
            if (nameLower.find(queryLower) == std::string::npos) continue;
        }

        visibleGames.push_back(&g);
    }
    
    if (currentTab == 2) {
        std::sort(visibleGames.begin(), visibleGames.end(), [](Game* a, Game* b){
            auto itA = std::find(recents.begin(), recents.end(), a->filename);
            auto itB = std::find(recents.begin(), recents.end(), b->filename);
            return std::distance(recents.begin(), itA) < std::distance(recents.begin(), itB);
        });
    }

    if (visibleGames.empty()) selectedIdx = 0;
    else if (selectedIdx >= (int)visibleGames.size()) selectedIdx = 0;
}

void launchGame(Game* g) {
    auto it = std::find(recents.begin(), recents.end(), g->filename);
    if (it != recents.end()) recents.erase(it);
    recents.push_front(g->filename);
    if (recents.size() > 5) recents.pop_back();
    saveData();

    def_prog_mode(); 
    endwin();        

    std::string cmd = EXEC_DIR + std::string("/") + g->filename;
    system(cmd.c_str());

    reset_prog_mode();
    refresh();
    
    scanGames();
    filterGames();
}

void printCentered(int y, std::string text, int attr = 0) {
    int visLen = utf8_visual_length(text);
    int x = (COLS - visLen) / 2;
    if (x < 0) x = 0;
    
    if (attr) attron(attr);
    mvprintw(y, x, "%s", text.c_str());
    if (attr) attroff(attr);
}

void drawBox(int y, int x, int h, int w) {
    mvhline(y, x + 1, ACS_HLINE, w - 2);
    mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);
    mvvline(y + 1, x, ACS_VLINE, h - 2);
    mvvline(y + 1, x + w - 1, ACS_VLINE, h - 2);
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
}

void drawUI() {
    clear();

    attron(COLOR_PAIR(1) | A_BOLD);
    printCentered(2, "╔═╗╔═╗╔╗╔╔═╗╔═╗╦  ╔═╗  ╦  ╔═╗╦ ╦╔╗╔╔═╗╦ ╦╔═╗╦═╗");
    printCentered(3, "║  ║ ║║║║╚═╗║ ║║  ║╣   ║  ╠═╣║ ║║║║║  ╠═╣║╣ ╠╦╝");
    printCentered(4, "╚═╝╚═╝╝╚╝╚═╝╚═╝╩═╝╚═╝  ╩═╝╩ ╩╚═╝╝╚╝╚═╝╩ ╩╚═╝╩╚═");
    attroff(COLOR_PAIR(1) | A_BOLD);

    int startY = 7;
    int tabW = 15;
    int totalTabW = tabW * 3;
    int startX = (COLS - totalTabW) / 2;
    
    const char* tabs[3] = {"ALL GAMES", "FAVORITES", "RECENTS"};
    for(int i=0; i<3; i++) {
        if (i == currentTab) attron(A_REVERSE | A_BOLD);
        mvprintw(startY, startX + i*tabW + (tabW - strlen(tabs[i]))/2, "%s", tabs[i]);
        if (i == currentTab) attroff(A_REVERSE | A_BOLD);
    }
    mvhline(startY + 1, 2, ACS_HLINE, COLS - 4);

    if (searchMode || !searchQuery.empty()) {
        attron(COLOR_PAIR(4));
        mvprintw(startY + 3, (COLS/2) - 10, "Search: %s_", searchQuery.c_str());
        attroff(COLOR_PAIR(4));
    }

    int listY = startY + 5;

    if (visibleGames.empty()) {
        attron(COLOR_PAIR(3));
        printCentered(listY + 2, "No games found in this category.");
        attroff(COLOR_PAIR(3));
    } else {
        if (!viewBlockMode) {
            int maxItems = LINES - listY - 5;
            if (maxItems < 1) maxItems = 1;
            int startIdx = 0;
            if (selectedIdx > maxItems - 1) startIdx = selectedIdx - (maxItems - 1);

            for (int i = 0; i < maxItems && (startIdx + i) < (int)visibleGames.size(); i++) {
                int idx = startIdx + i;
                Game* g = visibleGames[idx];
                
                int itemY = listY + i;
                int itemX = (COLS / 2) - 20;

                if (idx == selectedIdx) {
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvprintw(itemY, itemX - 2, ">");
                }
                
                mvprintw(itemY, itemX, "%s", g->displayName.c_str());
                
                if (g->isFavorite) {
                    attron(COLOR_PAIR(4));
                    mvprintw(itemY, itemX + 25, "★");
                    if (idx != selectedIdx) attroff(COLOR_PAIR(4));
                }

                if (idx == selectedIdx) attroff(COLOR_PAIR(2) | A_BOLD);
            }
            
            if (!visibleGames.empty()) {
                int descY = LINES - 4;
                attron(COLOR_PAIR(5));
                printCentered(descY, visibleGames[selectedIdx]->description);
                attroff(COLOR_PAIR(5));
            }

        } else {
            int cols = 3; 
            int boxW = 22;
            int boxH = 5;
            int spacingX = 2;
            int spacingY = 1;
            
            int totalGridW = (cols * boxW) + ((cols - 1) * spacingX);
            int gridStartX = (COLS - totalGridW) / 2;

            int maxRows = (LINES - listY - 3) / (boxH + spacingY);
            if (maxRows < 1) maxRows = 1;
            int itemsPerPage = maxRows * cols;
            
            int page = selectedIdx / itemsPerPage;
            int startItem = page * itemsPerPage;

            for (int i = 0; i < itemsPerPage && (startItem + i) < (int)visibleGames.size(); i++) {
                int idx = startItem + i;
                Game* g = visibleGames[idx];

                int row = i / cols;
                int col = i % cols;
                
                int bY = listY + row * (boxH + spacingY);
                int bX = gridStartX + col * (boxW + spacingX);

                if (idx == selectedIdx) attron(COLOR_PAIR(2) | A_BOLD);
                
                drawBox(bY, bX, boxH, boxW);
                
                int nameX = bX + (boxW - g->displayName.length()) / 2;
                mvprintw(bY + 2, nameX, "%s", g->displayName.c_str());

                if (g->isFavorite) {
                    attron(COLOR_PAIR(4));
                    mvprintw(bY + 1, bX + boxW - 2, "★");
                    if (idx != selectedIdx) attroff(COLOR_PAIR(4)); 
                    else attron(COLOR_PAIR(2)); 
                }
                
                if (idx == selectedIdx) attroff(COLOR_PAIR(2) | A_BOLD);
            }
        }
    }

    attron(COLOR_PAIR(5));
    mvhline(LINES - 2, 0, ACS_HLINE, COLS);
    mvprintw(LINES - 1, 2, "[ENTER] Play | [F] Fav | [/] Search | [V] View | [TAB] Category | [Q] Quit");
    attroff(COLOR_PAIR(5));
    
    refresh();
}

int main() {
    setlocale(LC_ALL, ""); 
    loadData();
    scanGames();
    filterGames();

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_BLUE, COLOR_BLACK);
    }

    while (true) {
        drawUI();
        int ch = getch();

        if (searchMode) {
            if (ch == 10) {
                searchMode = false; 
            } else if (ch == 27) {
                searchMode = false;
                searchQuery = "";
                filterGames();
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == KEY_DC) {
                if (!searchQuery.empty()) {
                    searchQuery.pop_back();
                    filterGames();
                }
            } else if (isalnum(ch) || ch == ' ' || ch == '-' || ch == '.') {
                searchQuery += (char)ch;
                currentTab = 0;
                filterGames();
            }
            continue; 
        }

        switch(ch) {
            case 'q': case 'Q':
                saveData();
                endwin();
                return 0;
            
            case KEY_UP: case 'w': 
                if (viewBlockMode) selectedIdx -= 3; 
                else selectedIdx--; 
                break;
            case KEY_DOWN: case 's': 
                if (viewBlockMode) selectedIdx += 3; 
                else selectedIdx++; 
                break;
            case KEY_LEFT: case 'a':
                if (viewBlockMode) selectedIdx--;
                else { currentTab--; if (currentTab < 0) currentTab = 2; filterGames(); }
                break;
            case KEY_RIGHT: case 'd':
                if (viewBlockMode) selectedIdx++;
                else { currentTab++; if (currentTab > 2) currentTab = 0; filterGames(); }
                break;
            case 9:
                currentTab++; 
                if (currentTab > 2) currentTab = 0; 
                filterGames(); 
                break;

            case 10:
                if (!visibleGames.empty()) {
                    launchGame(visibleGames[selectedIdx]);
                }
                break;
            
            case 'f': case 'F':
                if (!visibleGames.empty()) {
                    Game* g = visibleGames[selectedIdx];
                    if (g->isFavorite) favorites.erase(g->filename);
                    else favorites.insert(g->filename);
                    g->isFavorite = !g->isFavorite;
                    saveData();
                }
                break;

            case 'v': case 'V':
                viewBlockMode = !viewBlockMode;
                break;

            case '/':
                searchMode = true;
                currentTab = 0; 
                filterGames();
                break;

            case 'S':
                searchMode = true;
                currentTab = 0;
                filterGames();
                break;
        }

        if (!visibleGames.empty()) {
            if (selectedIdx < 0) selectedIdx = visibleGames.size() - 1;
            if (selectedIdx >= (int)visibleGames.size()) selectedIdx = 0;
        } else {
            selectedIdx = 0;
        }
    }

    endwin();
    return 0;
}
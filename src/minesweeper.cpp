#include <ncurses.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <clocale>
#include <string>
#include <algorithm>
#include <cstring>

struct Cell {
    bool isMine = false;
    bool isOpen = false;
    bool isFlagged = false;
    int neighbors = 0;
};

struct GameConfig {
    int width = 20;
    int height = 10;
    int mines = 25;

    int menuIndex = 0;
    int settingsIndex = 0;
};

GameConfig config;
std::vector<std::vector<Cell>> board;
int cursorX = 0, cursorY = 0;
bool gameOver = false;
bool victory = false;

void drawBox(int y, int x, int h, int w, const char* title = nullptr) {
    attron(COLOR_PAIR(1));
    mvprintw(y, x, "┌");
    mvprintw(y, x + w - 1, "┐");
    mvprintw(y + h - 1, x, "└");
    mvprintw(y + h - 1, x + w - 1, "┘");
    for (int i = 1; i < w - 1; i++) {
        mvprintw(y, x + i, "─");
        mvprintw(y + h - 1, x + i, "─");
    }
    for (int i = 1; i < h - 1; i++) {
        mvprintw(y + i, x, "│");
        mvprintw(y + i, x + w - 1, "│");
    }

    if (title) {
        int titleLen = strlen(title);
        int titleX = x + (w - titleLen) / 2;
        mvprintw(y, titleX - 1, " ");
        mvprintw(y, titleX, "%s", title);
        mvprintw(y, titleX + titleLen, " ");
    }
    attroff(COLOR_PAIR(1));
}

void printCentered(int y, std::string text, int attrs = 0) {
    int midX = COLS / 2;
    int startX = midX - (text.length() / 2);
    if (attrs) attron(attrs);
    mvprintw(y, startX, "%s", text.c_str());
    if (attrs) attroff(attrs);
}

bool isValid(int y, int x) {
    return (y >= 0 && y < config.height && x >= 0 && x < config.width);
}

void calculateNumbers() {
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            if (board[y][x].isMine) continue;

            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (isValid(y + dy, x + dx) && board[y + dy][x + dx].isMine) {
                        count++;
                    }
                }
            }
            board[y][x].neighbors = count;
        }
    }
}

void initGame() {
    board.clear();
    board.resize(config.height, std::vector<Cell>(config.width));

    cursorX = config.width / 2;
    cursorY = config.height / 2;
    gameOver = false;
    victory = false;

    int placed = 0;
    int maxMines = (config.width * config.height) - 1;
    if (config.mines > maxMines) config.mines = maxMines;

    while (placed < config.mines) {
        int ry = rand() % config.height;
        int rx = rand() % config.width;
        if (!board[ry][rx].isMine && (ry != cursorY || rx != cursorX)) {
            board[ry][rx].isMine = true;
            placed++;
        }
    }

    calculateNumbers();
}

void reveal(int y, int x) {
    if (!isValid(y, x)) return;
    if (board[y][x].isOpen || board[y][x].isFlagged) return;

    board[y][x].isOpen = true;

    if (board[y][x].neighbors == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx != 0 || dy != 0) { 
                    reveal(y + dy, x + dx);
                }
            }
        }
    }
}

void actionOpen() {
    if (board[cursorY][cursorX].isFlagged) return;

    if (board[cursorY][cursorX].isMine) {
        gameOver = true;
        for(int i=0; i<config.height; i++)
            for(int j=0; j<config.width; j++)
                if(board[i][j].isMine) board[i][j].isOpen = true;
    } else {
        reveal(cursorY, cursorX);
    }
}

void actionFlag() {
    if (!board[cursorY][cursorX].isOpen) {
        board[cursorY][cursorX].isFlagged = !board[cursorY][cursorX].isFlagged;
    }
}

void checkWin() {
    int openedCount = 0;
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            if (board[y][x].isOpen) openedCount++;
        }
    }
    if (openedCount == (config.width * config.height - config.mines)) {
        victory = true;
    }
}

std::string getSettingLabel(int index) {
    char buffer[50];
    switch(index) {
        case 0: snprintf(buffer, sizeof(buffer), "Width:  < %d >", config.width); break;
        case 1: snprintf(buffer, sizeof(buffer), "Height: < %d >", config.height); break;
        case 2: snprintf(buffer, sizeof(buffer), "Mines:  < %d >", config.mines); break;
        case 3: return "Back to Menu";
        default: return "";
    }
    return std::string(buffer);
}

void changeSetting(int index, int delta) {
    switch(index) {
        case 0:
            config.width += delta;
            if (config.width < 5) config.width = 5;
            if (config.width > 60) config.width = 60;
            break;
        case 1:
            config.height += delta;
            if (config.height < 5) config.height = 5;
            if (config.height > 30) config.height = 30;
            break;
        case 2:
            config.mines += delta;
            if (config.mines < 1) config.mines = 1;
            if (config.mines >= config.width * config.height) 
                config.mines = config.width * config.height - 1;
            break;
    }
}

void showSettings() {
    while (true) {
        clear();
        int midY = LINES / 2;
        int midX = COLS / 2;
        int boxW = 30, boxH = 12;

        drawBox(midY - boxH/2, midX - boxW/2, boxH, boxW, "SETTINGS");

        for (int i = 0; i < 4; i++) {
            std::string label = getSettingLabel(i);
            int y = midY - 2 + i * 2;
            int attr = (i == config.settingsIndex) ? A_REVERSE : 0;
            printCentered(y, label, attr);
        }

        mvprintw(midY + boxH/2 + 1, midX - 12, "Left/Right to change");

        int ch = getch();
        switch(ch) {
            case KEY_UP: case 'w': 
                if (config.settingsIndex > 0) config.settingsIndex--; 
                break;
            case KEY_DOWN: case 's': 
                if (config.settingsIndex < 3) config.settingsIndex++; 
                break;
            case KEY_LEFT: case 'a': 
                if (config.settingsIndex < 3) changeSetting(config.settingsIndex, -1); 
                break;
            case KEY_RIGHT: case 'd': 
                if (config.settingsIndex < 3) changeSetting(config.settingsIndex, 1); 
                break;
            case 10:
                if (config.settingsIndex == 3) return;
                break;
        }
    }
}

void showMenu() {
    const char* options[3] = {"Start Game", "Settings", "Exit"};

    while (true) {
        clear();
        int midY = LINES / 2;
        int midX = COLS / 2;
        int boxW = 30, boxH = 12;

        drawBox(midY - boxH/2, midX - boxW/2, boxH, boxW);
        printCentered(midY - 4, "=== MINESWEEPER ===");

        for (int i = 0; i < 3; i++) {
            int attr = (i == config.menuIndex) ? A_REVERSE : 0;
            printCentered(midY - 1 + i * 2, options[i], attr);
        }

        int ch = getch();
        switch(ch) {
            case KEY_UP: case 'w': if (config.menuIndex > 0) config.menuIndex--; break;
            case KEY_DOWN: case 's': if (config.menuIndex < 2) config.menuIndex++; break;
            case 10: 
                if (config.menuIndex == 0) return;
                if (config.menuIndex == 1) showSettings();
                if (config.menuIndex == 2) { endwin(); exit(0); }
                break;
        }
    }
}

void draw() {
    clear();
    int startY = (LINES - config.height) / 2;
    int startX = (COLS - config.width * 2) / 2;

    drawBox(startY - 1, startX - 1, config.height + 2, config.width * 2 + 2);

    mvprintw(startY - 2, startX, "Mines: %d", config.mines);
    mvprintw(startY + config.height + 1, startX, "[SPACE] Open  [F] Flag  [Q] Quit");

    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            int drawY = startY + y;
            int drawX = startX + x * 2;
            bool isCursor = (y == cursorY && x == cursorX);

            if (isCursor) attron(A_REVERSE);

            if (board[y][x].isFlagged) {
                attron(COLOR_PAIR(4)); 
                mvprintw(drawY, drawX, " F");
                attroff(COLOR_PAIR(4));
            } 
            else if (!board[y][x].isOpen) {
                mvprintw(drawY, drawX, " .");
            } 
            else {
                if (board[y][x].isMine) {
                    attron(COLOR_PAIR(5) | A_BOLD); 
                    mvprintw(drawY, drawX, " X");
                    attroff(COLOR_PAIR(5) | A_BOLD);
                } 
                else if (board[y][x].neighbors > 0) {
                    int color = (board[y][x].neighbors == 1) ? 2 : (board[y][x].neighbors == 2) ? 3 : 4;
                    attron(COLOR_PAIR(color));
                    mvprintw(drawY, drawX, " %d", board[y][x].neighbors);
                    attroff(COLOR_PAIR(color));
                } 
                else {
                    mvprintw(drawY, drawX, "  ");
                }
            }

            if (isCursor) attroff(A_REVERSE);
        }
    }
    refresh();
}

int main() {
    setlocale(LC_ALL, ""); 
    srand(time(0));

    initscr();
    cbreak();
    noecho();
    curs_set(0); 
    keypad(stdscr, TRUE); 

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_BLUE, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_RED, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    }

    while (true) {
        showMenu();
        initGame();

        while (true) {
            draw();

            if (gameOver || victory) {
                int midY = LINES / 2;

                attron(COLOR_PAIR(1) | A_BOLD);

                if (victory) printCentered(midY, " VICTORY! ", A_REVERSE | A_BOLD);
                else printCentered(midY, " GAME OVER ", A_REVERSE | A_BOLD);

                printCentered(midY + 1, " Press ENTER to menu ");
                attroff(COLOR_PAIR(1) | A_BOLD);

                refresh();
                while (getch() != 10);
                break;
            }

            int ch = getch();
            switch (ch) {
                case KEY_LEFT:  case 'a': if (cursorX > 0) cursorX--; break;
                case KEY_RIGHT: case 'd': if (cursorX < config.width - 1) cursorX++; break;
                case KEY_UP:    case 'w': if (cursorY > 0) cursorY--; break;
                case KEY_DOWN:  case 's': if (cursorY < config.height - 1) cursorY++; break;
                case ' ':       actionOpen(); checkWin(); break; 
                case 'f':       actionFlag(); break; 
                case 'q':       gameOver = true; break;
            }
        }
    }

    endwin();
    return 0;
}
#include <ncurses.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <clocale>
#include <algorithm>
#include <string>
#include <random>

const int N = 9;

struct Cell {
    int val = 0;       
    bool fixed = false; 
    bool isPencil = false;
};

Cell grid[N][N];     
int solution[N][N];  
int cursorX = 0, cursorY = 0;
bool gameOver = false;
int difficulty = 40; 

time_t startTime;
int mistakes = 0;
const int MAX_MISTAKES = 3;
int hints = 3;
bool pencilMode = false;

bool isSafe(int grid[N][N], int row, int col, int num) {
    for (int x = 0; x < N; x++) if (grid[row][x] == num) return false;
    for (int x = 0; x < N; x++) if (grid[x][col] == num) return false;
    int startRow = row - row % 3, startCol = col - col % 3;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (grid[i + startRow][j + startCol] == num) return false;
    return true;
}

bool solveSudoku(int grid[N][N]) {
    int row, col;
    bool isEmpty = false;
    for (row = 0; row < N; row++) {
        for (col = 0; col < N; col++) {
            if (grid[row][col] == 0) { isEmpty = true; goto break_loop; }
        }
    }
    break_loop:
    if (!isEmpty) return true; 

    for (int num = 1; num <= 9; num++) {
        if (isSafe(grid, row, col, num)) {
            grid[row][col] = num;
            if (solveSudoku(grid)) return true;
            grid[row][col] = 0; 
        }
    }
    return false;
}

void generateGame() {
    int tempGrid[N][N] = {}; 
    std::random_device rd;
    std::mt19937 g(rd());

    mistakes = 0;
    hints = 3;
    pencilMode = false;
    startTime = time(0);

    for (int i = 0; i < N; i += 3) {
        std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::shuffle(nums.begin(), nums.end(), g);
        int idx = 0;
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 3; c++)
                tempGrid[i + r][i + c] = nums[idx++];
    }

    solveSudoku(tempGrid);

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            solution[i][j] = tempGrid[i][j];

    int holes = difficulty;
    while (holes > 0) {
        int r = rand() % N;
        int c = rand() % N;
        if (tempGrid[r][c] != 0) {
            tempGrid[r][c] = 0;
            holes--;
        }
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            grid[i][j].val = tempGrid[i][j];
            grid[i][j].fixed = (tempGrid[i][j] != 0);
            grid[i][j].isPencil = false;
        }
    }
}

bool checkWin() {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            if (grid[i][j].val != solution[i][j]) return false;
    return true;
}

void useHint() {
    if (hints <= 0) return;
    
    std::vector<std::pair<int, int>> candidates;
    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            if (!grid[i][j].fixed && grid[i][j].val != solution[i][j]) {
                candidates.push_back({i, j});
            }
        }
    }

    if (!candidates.empty()) {
        int idx = rand() % candidates.size();
        int r = candidates[idx].first;
        int c = candidates[idx].second;
        
        grid[r][c].val = solution[r][c];
        grid[r][c].fixed = false;
        grid[r][c].isPencil = false;
        hints--;
    }
}

void printCentered(int y, std::string text, int attrs = 0) {
    int startX = (COLS - text.length()) / 2;
    if (attrs) attron(attrs);
    mvprintw(y, startX, "%s", text.c_str());
    if (attrs) attroff(attrs);
}

void drawLine(int y, int startX, int type) {
    move(y, startX);
    if (type == 0) addstr("╔");
    else if (type == 3) addstr("╚");
    else if (type == 2) addstr("╠");
    else addstr("╟");

    for (int i = 0; i < 9; i++) {
        if (type == 1) addstr("───"); else addstr("═══");
        if (i == 8) { 
            if (type == 0) addstr("╗");
            else if (type == 3) addstr("╝");
            else if (type == 2) addstr("╣");
            else addstr("╢");
        } else if ((i + 1) % 3 == 0) {
            if (type == 0) addstr("╦");
            else if (type == 3) addstr("╩");
            else if (type == 2) addstr("╬");
            else addstr("╫");
        } else { 
            if (type == 0) addstr("╤");
            else if (type == 3) addstr("╧");
            else if (type == 2) addstr("╪");
            else addstr("┼");
        }
    }
}

void drawGrid() {
    int tableWidth = 37;
    int startY = (LINES - 21) / 2; 
    int startX = (COLS - tableWidth) / 2;
    
    time_t now = time(0);
    int elapsed = difftime(now, startTime);
    int mins = elapsed / 60;
    int secs = elapsed % 60;
    
    mvprintw(startY - 2, startX, "Mistakes: %d/%d", mistakes, MAX_MISTAKES);
    mvprintw(startY - 2, startX + tableWidth - 10, "Time: %02d:%02d", mins, secs);
    
    mvprintw(startY - 1, startX, "Hints: %d", hints);
    if (pencilMode) {
        attron(A_BOLD | COLOR_PAIR(5));
        mvprintw(startY - 1, startX + tableWidth - 11, "[ PENCIL ]");
        attroff(A_BOLD | COLOR_PAIR(5));
    } else {
        mvprintw(startY - 1, startX + tableWidth - 11, "[ NORMAL ]");
    }

    int currentY = startY;
    drawLine(currentY++, startX, 0);

    int cursorVal = grid[cursorY][cursorX].val;

    for (int i = 0; i < N; i++) {
        move(currentY, startX);
        addstr("║"); 

        for (int j = 0; j < N; j++) {
            int val = grid[i][j].val;
            bool isCursor = (i == cursorY && j == cursorX);
            bool isHighlighted = (val != 0 && val == cursorVal);
            int colorPair = 1; 

            if (grid[i][j].fixed) {
                colorPair = 2;
            } else if (grid[i][j].isPencil) {
                colorPair = 5;
            } else if (val != 0 && val != solution[i][j]) {
                colorPair = 3;
            } else if (val != 0 && val == solution[i][j]) {
                colorPair = 4; 
            }
            
            if (isCursor) {
                attron(A_REVERSE);
            } else if (isHighlighted && !grid[i][j].isPencil) {
                attron(COLOR_PAIR(6)); 
            }

            if (!(isHighlighted && !isCursor && !grid[i][j].isPencil)) {
                attron(COLOR_PAIR(colorPair));
            }
            
            if (grid[i][j].isPencil) attron(A_DIM);

            if (val == 0) printw(" . ");
            else printw(" %d ", val);
            
            if (grid[i][j].isPencil) attroff(A_DIM);
            if (!(isHighlighted && !isCursor && !grid[i][j].isPencil)) attroff(COLOR_PAIR(colorPair));
            if (isHighlighted && !grid[i][j].isPencil && !isCursor) attroff(COLOR_PAIR(6));
            if (isCursor) attroff(A_REVERSE);

            if ((j + 1) % 3 == 0) addstr("║"); 
            else addstr("│"); 
        }
        currentY++;

        if (i < N - 1) {
            if ((i + 1) % 3 == 0) drawLine(currentY++, startX, 2); 
            else drawLine(currentY++, startX, 1); 
        }
    }

    drawLine(currentY++, startX, 3);
    
    mvprintw(currentY, startX, "[ARROWS] Move | [1-9] Input | [0/DEL] Clear");
    mvprintw(currentY + 1, startX, "[P] Pencil Mode | [H] Hint | [Q] Quit");
}

void showMenu() {
    int selected = 0;
    const char* options[4] = {"Easy", "Normal", "Hard", "Exit"};
    
    while (true) {
        clear();
        int midY = LINES / 2;
        int boxW = 20, boxH = 10;
        int boxX = (COLS - boxW) / 2;
        int boxY = midY - 5;
        
        attron(COLOR_PAIR(2));
        mvprintw(boxY, boxX, "┌──────────────────┐");
        for(int k=1; k<boxH-1; k++) mvprintw(boxY+k, boxX, "│                  │");
        mvprintw(boxY+boxH-1, boxX, "└──────────────────┘");
        attroff(COLOR_PAIR(2));

        printCentered(midY - 3, "SUDOKU", A_BOLD);

        for (int i = 0; i < 4; i++) {
            int attr = (i == selected) ? A_REVERSE : 0;
            printCentered(midY - 1 + i, options[i], attr);
        }

        int ch = getch();
        switch(ch) {
            case KEY_UP: case 'w': if (selected > 0) selected--; break;
            case KEY_DOWN: case 's': if (selected < 3) selected++; break;
            case 10: 
                if (selected == 0) difficulty = 30;
                if (selected == 1) difficulty = 40;
                if (selected == 2) difficulty = 55;
                if (selected == 3) { endwin(); exit(0); }
                return;
        }
    }
}

int main() {
    setlocale(LC_ALL, ""); 
    srand(time(0));

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    }

    while (true) {
        nodelay(stdscr, FALSE);
        showMenu();
        generateGame();
        
        nodelay(stdscr, TRUE);
        cursorX = 0; cursorY = 0;
        gameOver = false;
        
        while (!gameOver) {
            drawGrid();

            if (checkWin()) {
                nodelay(stdscr, FALSE);
                attron(COLOR_PAIR(2) | A_BOLD);
                printCentered(LINES/2, " VICTORY! ", A_REVERSE);
                printCentered(LINES/2 + 1, " Press ENTER ", A_REVERSE);
                attroff(COLOR_PAIR(2) | A_BOLD);
                refresh();
                while(getch() != 10);
                break;
            }
            
            if (mistakes >= MAX_MISTAKES) {
                nodelay(stdscr, FALSE);
                attron(COLOR_PAIR(3) | A_BOLD);
                printCentered(LINES/2, " GAME OVER (Too many mistakes) ", A_REVERSE);
                printCentered(LINES/2 + 1, " Press ENTER ", A_REVERSE);
                attroff(COLOR_PAIR(3) | A_BOLD);
                refresh();
                while(getch() != 10);
                break;
            }

            int ch = getch();
            
            if (ch == ERR) {
                napms(100);
                continue;
            }

            switch (ch) {
                case KEY_LEFT: case 'a':  cursorX = (cursorX - 1 + 9) % 9; break;
                case KEY_RIGHT: case 'd': cursorX = (cursorX + 1) % 9; break;
                case KEY_UP: case 'w':    cursorY = (cursorY - 1 + 9) % 9; break;
                case KEY_DOWN: case 's':  cursorY = (cursorY + 1) % 9; break;
                case 'q': gameOver = true; break; 
                
                case 'p': case 'P': pencilMode = !pencilMode; break;
                case 'h': case 'H': useHint(); break;

                case '1': case '2': case '3':
                case '4': case '5': case '6':
                case '7': case '8': case '9':
                    if (!grid[cursorY][cursorX].fixed) {
                        int num = ch - '0';
                        grid[cursorY][cursorX].val = num;
                        
                        if (pencilMode) {
                            grid[cursorY][cursorX].isPencil = true;
                        } else {
                            grid[cursorY][cursorX].isPencil = false;
                            if (num != solution[cursorY][cursorX]) {
                                mistakes++;
                            }
                        }
                    }
                    break;
                case '0': case KEY_BACKSPACE: case 127: case KEY_DC:
                    if (!grid[cursorY][cursorX].fixed) {
                        grid[cursorY][cursorX].val = 0;
                        grid[cursorY][cursorX].isPencil = false;
                    }
                    break;
            }
        }
    }

    endwin();
    return 0;
}

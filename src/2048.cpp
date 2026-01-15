#include <ncurses.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include <cstring>
#include <clocale>

const int GRID_SIZE = 4;
const int CELL_WIDTH = 10;
const int CELL_HEIGHT = 5;
const int WIN_VALUE = 2048;

int board[GRID_SIZE][GRID_SIZE];
int score = 0;
int highScore = 0;
bool gameOver = false;
bool victory = false;

const char* HIGHSCORE_FILE = "highscore_2048.txt";

void loadHighScore() {
    FILE* file = fopen(HIGHSCORE_FILE, "r");
    if (file) {
        if (fscanf(file, "%d", &highScore) != 1) highScore = 0;
        fclose(file);
    } else {
        highScore = 0;
    }
}

void saveHighScore() {
    if (score > highScore) {
        highScore = score;
        FILE* file = fopen(HIGHSCORE_FILE, "w");
        if (file) {
            fprintf(file, "%d", highScore);
            fclose(file);
        }
    }
}

void spawnTile() {
    std::vector<std::pair<int, int>> emptyCells;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (board[i][j] == 0) emptyCells.push_back({i, j});
        }
    }

    if (!emptyCells.empty()) {
        int idx = rand() % emptyCells.size();
        board[emptyCells[idx].first][emptyCells[idx].second] = (rand() % 10 == 0) ? 4 : 2;
    }
}

void initGame() {
    score = 0;
    gameOver = false;
    victory = false;
    for (int i = 0; i < GRID_SIZE; i++)
        for (int j = 0; j < GRID_SIZE; j++)
            board[i][j] = 0;

    spawnTile();
    spawnTile();
}

bool canMove() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (board[i][j] == 0) return true;
            if (j < GRID_SIZE - 1 && board[i][j] == board[i][j + 1]) return true;
            if (i < GRID_SIZE - 1 && board[i][j] == board[i + 1][j]) return true;
        }
    }
    return false;
}

std::vector<int> processLine(const std::vector<int>& line, bool& moved) {
    std::vector<int> newLine;
    for (int num : line) {
        if (num != 0) newLine.push_back(num);
    }

    for (size_t i = 0; i + 1 < newLine.size(); i++) {
        if (newLine[i] == newLine[i + 1]) {
            newLine[i] *= 2;
            score += newLine[i];
            newLine[i + 1] = 0;
            if (newLine[i] == WIN_VALUE) victory = true;
            i++; 
        }
    }

    std::vector<int> finalLine;
    for (int num : newLine) {
        if (num != 0) finalLine.push_back(num);
    }

    while (finalLine.size() < GRID_SIZE) {
        finalLine.push_back(0);
    }

    for(size_t i=0; i<GRID_SIZE; i++) {
        if(finalLine[i] != line[i]) moved = true;
    }

    return finalLine;
}

void move(int dir) {
    bool moved = false;

    if (dir == 0 || dir == 1) { 
        for (int i = 0; i < GRID_SIZE; i++) {
            std::vector<int> line;
            if (dir == 0) {
                for (int j = 0; j < GRID_SIZE; j++) line.push_back(board[i][j]);
                line = processLine(line, moved);
                for (int j = 0; j < GRID_SIZE; j++) board[i][j] = line[j];
            } else {
                for (int j = GRID_SIZE - 1; j >= 0; j--) line.push_back(board[i][j]);
                line = processLine(line, moved);
                for (int j = 0; j < GRID_SIZE; j++) board[i][GRID_SIZE - 1 - j] = line[j];
            }
        }
    } else { 
        for (int j = 0; j < GRID_SIZE; j++) {
            std::vector<int> line;
            if (dir == 2) {
                for (int i = 0; i < GRID_SIZE; i++) line.push_back(board[i][j]);
                line = processLine(line, moved);
                for (int i = 0; i < GRID_SIZE; i++) board[i][j] = line[i];
            } else {
                for (int i = GRID_SIZE - 1; i >= 0; i--) line.push_back(board[i][j]);
                line = processLine(line, moved);
                for (int i = 0; i < GRID_SIZE; i++) board[GRID_SIZE - 1 - i][j] = line[i];
            }
        }
    }

    if (moved) {
        spawnTile();
        if (!canMove()) gameOver = true;
    }
}

int getColorPair(int value) {
    if (value == 0) return 1;    
    if (value == 2) return 2;    
    if (value == 4) return 3;    
    if (value == 8) return 4;    
    if (value == 16) return 5;   
    if (value == 32) return 6;   
    if (value == 64) return 7;   
    return 8;                    
}

void drawBox(int y, int x, int h, int w) {
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
}

void draw() {
    clear();
    int boardWidth = GRID_SIZE * CELL_WIDTH;
    int boardHeight = GRID_SIZE * CELL_HEIGHT;
    int startY = (LINES - boardHeight) / 2;
    int startX = (COLS - boardWidth) / 2;

    mvprintw(startY - 4, startX, "SCORE: %d", score);
    mvprintw(startY - 4, startX + boardWidth - 15, "HIGH: %d", highScore);
    mvprintw(startY - 2, startX, "Use WASD or Arrows to slide.");

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int y = startY + i * CELL_HEIGHT;
            int x = startX + j * CELL_WIDTH;
            int val = board[i][j];

            int color = getColorPair(val);
            attron(COLOR_PAIR(color));
            
            drawBox(y, x, CELL_HEIGHT, CELL_WIDTH);

            if (val != 0) {
                std::string s = std::to_string(val);
                int textX = x + (CELL_WIDTH - s.length()) / 2;
                int textY = y + CELL_HEIGHT / 2;
                mvprintw(textY, textX, "%s", s.c_str());
            }
            attroff(COLOR_PAIR(color));
        }
    }

    refresh();
}

void printCentered(int y, std::string text, bool highlight = false) {
    int midX = COLS / 2;
    int startX = midX - (text.length() / 2);
    if (highlight) attron(A_REVERSE);
    mvprintw(y, startX, "%s", text.c_str());
    if (highlight) attroff(A_REVERSE);
}

void showMenu() {
    int selected = 0;
    const char* options[2] = {"Start Game", "Exit"};
    
    while (true) {
        clear();
        int midY = LINES / 2;
        int midX = COLS / 2;

        int w = 32, h = 12;
        int boxY = midY - h/2;
        int boxX = midX - w/2;
        
        attron(COLOR_PAIR(6)); 
        drawBox(boxY, boxX, h, w);
        attroff(COLOR_PAIR(6));

        printCentered(midY - 3, "=== 2048 ===");
        
        char scoreBuf[40];
        snprintf(scoreBuf, sizeof(scoreBuf), "High Score: %d", highScore);
        printCentered(midY - 1, scoreBuf);

        for (int i = 0; i < 2; i++) {
            printCentered(midY + 2 + i * 2, options[i], (i == selected));
        }

        int c = getch();
        switch (c) {
            case KEY_UP: case 'w': if (selected > 0) selected--; break;
            case KEY_DOWN: case 's': if (selected < 1) selected++; break;
            case 10: 
                if (selected == 0) return; 
                if (selected == 1) { endwin(); exit(0); }
                break;
        }
    }
}

int main() {
    setlocale(LC_ALL, "");
    srand(time(0));
    loadHighScore();

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK);   
        init_pair(2, COLOR_WHITE, COLOR_BLACK);   
        init_pair(3, COLOR_CYAN, COLOR_BLACK);    
        init_pair(4, COLOR_GREEN, COLOR_BLACK);   
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);  
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK); 
        init_pair(7, COLOR_RED, COLOR_BLACK);     
        init_pair(8, COLOR_BLUE, COLOR_BLACK);    
    }

    while (true) {
        nodelay(stdscr, FALSE);
        showMenu();

        initGame();
        nodelay(stdscr, FALSE);

        while (!gameOver && !victory) {
            draw();
            int ch = getch();
            switch(ch) {
                case KEY_LEFT:  case 'a': move(0); break;
                case KEY_RIGHT: case 'd': move(1); break;
                case KEY_UP:    case 'w': move(2); break;
                case KEY_DOWN:  case 's': move(3); break;
                case 'q': gameOver = true; break;
            }
        }

        draw(); 
        saveHighScore();
        
        attron(COLOR_PAIR(7)); 
        int midY = LINES / 2;
        if (victory) printCentered(midY, "YOU WIN! (2048 Reached)");
        else printCentered(midY, "GAME OVER!");
        
        printCentered(midY + 1, "Press ENTER to continue");
        attroff(COLOR_PAIR(7));
        refresh();

        while (getch() != 10);
    }

    endwin();
    return 0;
}

#include <ncurses.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fstream>
#include <string>
#include <cstring>

const char* HIGHSCORE_FILE = "highscore.txt";
const int MENU_WIDTH = 40;  
const int MENU_HEIGHT = 16; 

struct Point {
    int x, y;
};

struct GameConfig {
    int width = 40;
    int height = 20;
    int speedDelay = 90000; 
    int foodCount = 1;
    bool godMode = false;
    
    int sizeIndex = 1;  
    int speedIndex = 1; 
};

GameConfig config;
int score = 0;
int highScore = 0;
bool gameOver = false;
int dirX = 1, dirY = 0;

std::vector<Point> fruits;
std::vector<Point> snake;

void loadHighScore() {
    std::ifstream file(HIGHSCORE_FILE);
    if (file.is_open()) {
        file >> highScore;
        file.close();
    } else {
        highScore = 0;
    }
}

void saveHighScore() {
    if (config.godMode) return;

    if (score > highScore) {
        highScore = score;
        std::ofstream file(HIGHSCORE_FILE);
        if (file.is_open()) {
            file << highScore;
            file.close();
        }
    }
}

Point getRandomEmptyPosition() {
    Point p;
    bool occupied;
    do {
        occupied = false;
        p.x = rand() % (config.width - 2) + 1;
        p.y = rand() % (config.height - 2) + 1;
        
        for (auto& s : snake) {
            if (s.x == p.x && s.y == p.y) occupied = true;
        }
        for (auto& f : fruits) {
            if (f.x == p.x && f.y == p.y) occupied = true;
        }
    } while (occupied);
    return p;
}

void initGame() {
    score = 0;
    dirX = 1; dirY = 0;
    snake.clear();
    fruits.clear();
    
    int startX = config.width / 2;
    int startY = config.height / 2;
    snake.push_back({startX, startY});
    snake.push_back({startX - 1, startY});
    snake.push_back({startX - 2, startY});
    
    for (int i = 0; i < config.foodCount; i++) {
        fruits.push_back(getRandomEmptyPosition());
    }
}

void drawBox(int y, int x, int h, int w, const char* title = nullptr) {
    attron(COLOR_PAIR(3));
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
    
    attroff(COLOR_PAIR(3));
}

void printCentered(int y, std::string text, bool highlight = false) {
    int midX = COLS / 2;
    int startX = midX - (text.length() / 2);
    
    if (highlight) attron(A_REVERSE);
    mvprintw(y, startX, "%s", text.c_str());
    if (highlight) attroff(A_REVERSE);
}

void draw() {
    clear();
    int offsetX = (COLS - config.width) / 2;
    int offsetY = (LINES - config.height) / 2;

    drawBox(offsetY, offsetX, config.height, config.width);

    attron(COLOR_PAIR(1)); 
    for (size_t i = 0; i < snake.size(); i++) {
        if (i == 0) {
            if (config.godMode) attron(COLOR_PAIR(4)); 
            mvprintw(offsetY + snake[i].y, offsetX + snake[i].x, "▓");
            if (config.godMode) attroff(COLOR_PAIR(4));
        } else {
            mvprintw(offsetY + snake[i].y, offsetX + snake[i].x, "▒");
        }
    }
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2)); 
    for (auto& f : fruits) {
        mvprintw(offsetY + f.y, offsetX + f.x, "*");
    }
    attroff(COLOR_PAIR(2));

    int infoY = offsetY + config.height + 1;
    mvprintw(infoY, offsetX, "Score: %d", score);
    mvprintw(infoY, offsetX + 15, "High: %d", highScore);
    if (config.godMode) {
        attron(COLOR_PAIR(4));
        mvprintw(infoY, offsetX + 30, "[GOD MODE]");
        attroff(COLOR_PAIR(4));
    }

    refresh();
}

void input() {
    int ch = getch();
    switch (ch) {
        case KEY_LEFT: case 'a': if (dirX != 1) { dirX = -1; dirY = 0; } break;
        case KEY_RIGHT: case 'd': if (dirX != -1) { dirX = 1; dirY = 0; } break;
        case KEY_UP: case 'w': if (dirY != 1) { dirX = 0; dirY = -1; } break;
        case KEY_DOWN: case 's': if (dirY != -1) { dirX = 0; dirY = 1; } break;
        case 'q': gameOver = true; break;
    }
}

void logic() {
    Point newHead = {snake[0].x + dirX, snake[0].y + dirY};

    if (config.godMode) {
        if (newHead.x >= config.width - 1) newHead.x = 1;
        else if (newHead.x <= 0) newHead.x = config.width - 2;
        if (newHead.y >= config.height - 1) newHead.y = 1;
        else if (newHead.y <= 0) newHead.y = config.height - 2;
    } else {
        if (newHead.x <= 0 || newHead.x >= config.width - 1 || 
            newHead.y <= 0 || newHead.y >= config.height - 1) {
            gameOver = true;
            return;
        }
    }

    if (!config.godMode) {
        for (size_t i = 0; i < snake.size(); i++) {
            if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
                gameOver = true;
                return;
            }
        }
    }

    snake.insert(snake.begin(), newHead);

    bool ate = false;
    for (size_t i = 0; i < fruits.size(); i++) {
        if (newHead.x == fruits[i].x && newHead.y == fruits[i].y) {
            score += 10;
            ate = true;
            fruits[i] = getRandomEmptyPosition();
            break; 
        }
    }

    if (!ate) snake.pop_back();
}

void changeSetting(int option) {
    switch (option) {
        case 0: 
            config.sizeIndex = (config.sizeIndex + 1) % 3;
            if (config.sizeIndex == 0) { config.width = 20; config.height = 10; }
            else if (config.sizeIndex == 1) { config.width = 40; config.height = 20; }
            else { config.width = 60; config.height = 30; }
            break;
        case 1: 
            config.speedIndex = (config.speedIndex + 1) % 4;
            if (config.speedIndex == 0) config.speedDelay = 130000;
            else if (config.speedIndex == 1) config.speedDelay = 90000;
            else if (config.speedIndex == 2) config.speedDelay = 50000;
            else config.speedDelay = 25000;
            break;
        case 2: 
            config.foodCount++;
            if (config.foodCount > 5) config.foodCount = 1;
            break;
        case 3: 
            config.godMode = !config.godMode;
            break;
    }
}

std::string getSettingLabel(int option) {
    char buffer[60];
    switch (option) {
        case 0: 
            return (config.sizeIndex == 0) ? "Map Size: [ SMALL ]" : 
                   (config.sizeIndex == 1) ? "Map Size: [ NORMAL ]" : "Map Size: [ LARGE ]";
        case 1:
            return (config.speedIndex == 0) ? "Speed:    [ SLOW ]" :
                   (config.speedIndex == 1) ? "Speed:    [ NORMAL ]" :
                   (config.speedIndex == 2) ? "Speed:    [ FAST ]" : "Speed:    [ INSANE ]";
        case 2:
            snprintf(buffer, sizeof(buffer), "Stars:    [ %d ]", config.foodCount);
            return std::string(buffer);
        case 3:
            return std::string("God Mode: [ ") + (config.godMode ? "ON" : "OFF") + " ]";
        case 4:
            return "Back";
        default: return "";
    }
}

void showSettings() {
    int selected = 0;
    while (true) {
        clear();
        int midY = LINES / 2;
        int midX = COLS / 2;
        
        drawBox(midY - MENU_HEIGHT/2, midX - MENU_WIDTH/2, MENU_HEIGHT, MENU_WIDTH, "SETTINGS");

        for (int i = 0; i < 5; i++) {
            std::string label = getSettingLabel(i);
            printCentered(midY - 4 + i * 2, label, (i == selected));
        }

        int c = getch();
        switch (c) {
            case KEY_UP: case 'w': if (selected > 0) selected--; break;
            case KEY_DOWN: case 's': if (selected < 4) selected++; break;
            case 10: 
                if (selected == 4) return; 
                changeSetting(selected);
                break;
        }
    }
}

void showMenu() {
    int selected = 0;
    const char* options[3] = {"Start Game", "Settings", "Exit"};
    
    while (true) {
        clear();
        int midY = LINES / 2;
        int midX = COLS / 2;

        drawBox(midY - MENU_HEIGHT/2, midX - MENU_WIDTH/2, MENU_HEIGHT, MENU_WIDTH);
        printCentered(midY - 4, "=== SNAKE GAME ===");
        
        char scoreBuf[40];
        snprintf(scoreBuf, sizeof(scoreBuf), "High Score: %d", highScore);
        printCentered(midY - 2, scoreBuf);

        for (int i = 0; i < 3; i++) {
            printCentered(midY + 1 + i * 2, options[i], (i == selected));
        }

        int c = getch();
        switch (c) {
            case KEY_UP: case 'w': if (selected > 0) selected--; break;
            case KEY_DOWN: case 's': if (selected < 2) selected++; break;
            case 10: 
                if (selected == 0) return; 
                if (selected == 1) showSettings();
                if (selected == 2) { endwin(); exit(0); }
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
        init_pair(1, COLOR_GREEN, COLOR_BLACK);  
        init_pair(2, COLOR_RED, COLOR_BLACK);    
        init_pair(3, COLOR_CYAN, COLOR_BLACK);   
        init_pair(4, COLOR_YELLOW, COLOR_BLACK); 
    }

    while (true) {
        nodelay(stdscr, FALSE);
        showMenu();

        initGame();
        gameOver = false;
        nodelay(stdscr, TRUE); 

        while (!gameOver) {
            input();
            logic();
            draw();
            
            if (dirY != 0) usleep(config.speedDelay * 1.7);
            else usleep(config.speedDelay);
        }

        saveHighScore();
        nodelay(stdscr, FALSE);
        
        attron(COLOR_PAIR(2));
        printCentered(LINES / 2, "GAME OVER!");
        
        if (config.godMode) {
            printCentered(LINES / 2 + 1, "(Score not saved: God Mode active)");
            printCentered(LINES / 2 + 2, "Press ENTER for Menu");
        } else {
            printCentered(LINES / 2 + 1, "Press ENTER for Menu");
        }
        
        attroff(COLOR_PAIR(2));
        refresh();
        
        while (getch() != 10);
    }

    endwin();
    return 0;
}

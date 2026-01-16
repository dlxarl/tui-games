#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <ctime>

using namespace std;

#define RESET       "\033[0m"
#define YELLOW      "\033[1;33m"
#define BLUE        "\033[1;34m"
#define RED         "\033[1;31m"
#define CYAN        "\033[1;36m"
#define GREEN       "\033[1;32m"
#define WHITE       "\033[1;37m"
#define GRAY        "\033[1;30m"
#define BG_WHITE    "\033[47m"
#define BG_BLACK    "\033[40m"
#define BLACK_TXT   "\033[30m"

const string WALL_CHAR      = "██";
const string EMPTY_CHAR     = "  ";
const string DOT_CHAR       = " ·";
const string POWER_CHAR     = " ●";
const string GHOST_CHAR     = " &";
const int POWER_DURATION    = 50;

struct termios orig_termios;

void restoreTerminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    cout << "\033[?25h";
    cout << "\033[?1049l";
}

void setupTerminal() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(restoreTerminal);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    cout << "\033[?1049h";
    cout << "\033[?25l";
}

struct Point { int x, y; };

class PacmanGame {
    enum State { MENU, SETTINGS, GAME, GAME_OVER };
    State currentState;
    
    int menuSelection;
    int settingsSelection;

    vector<string> map;
    vector<string> initialMap;
    int w, h;
    int score;
    bool win;

    int setSpeedIndex; 
    int setGhostCount;
    int gameSpeedDelay;

    Point player;
    Point dir;
    Point nextDir;
    Point ghostSpawn;
    
    struct Ghost {
        Point pos;
        Point dir;
        bool isDead;
    };
    vector<Ghost> ghosts;

    bool isPowered;
    int powerTimer;

    int padTop, padLeft;

public:
    PacmanGame() {
        initialMap = {
            "###################",
            "#........#........#",
            "#.##.###.#.###.##.#",
            "#*................*",
            "#.##.#.#####.#.##.#",
            "#....#...#...#....#",
            "####.### # ###.####",
            "    .#   G   #.    ",
            "####.### # ###.####",
            "#........P........#",
            "#.##.###.#.###.##.#",
            "#*................*",
            "##.#.#.#####.#.#.##",
            "#....#...#...#....#",
            "#.######.#.######.#",
            "#.................#",
            "###################"
        };

        h = initialMap.size();
        w = initialMap[0].size();
        
        currentState = MENU;
        menuSelection = 0;
        settingsSelection = 0;
        
        setSpeedIndex = 1; 
        setGhostCount = 4;
        
        resetGame();
    }

    void resetGame() {
        map = initialMap;
        score = 0;
        win = false;
        dir = {0, 0};
        nextDir = {0, 0};
        isPowered = false;
        powerTimer = 0;
        
        ghosts.clear();

        vector<Point> spawnPoints;

        for(int y=0; y<h; y++) {
            for(int x=0; x<w; x++) {
                if(map[y][x] == 'P') {
                    player = {x, y};
                    map[y][x] = ' ';
                } else if(map[y][x] == 'G') {
                    ghostSpawn = {x, y};
                    spawnPoints.push_back({x, y});
                    map[y][x] = ' ';
                }
            }
        }

        for(int i=0; i<setGhostCount; i++) {
            // ВИПРАВЛЕНО WARNING: додано (size_t)
            Point p = ((size_t)i < spawnPoints.size()) ? spawnPoints[i] : ghostSpawn;
            ghosts.push_back({p, {0, 0}, false});
        }

        int speeds[] = { 130000, 90000, 60000, 30000 };
        gameSpeedDelay = speeds[setSpeedIndex];
    }

    void calculateLayout() {
        struct winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
        
        int gameWidth = (w * 2) + 4;
        int gameHeight = h + 6;

        padTop = (ws.ws_row - gameHeight) / 2;
        padLeft = (ws.ws_col - gameWidth) / 2;
        if(padTop < 0) padTop = 0;
        if(padLeft < 0) padLeft = 0;
    }

    void handleInput() {
        char buf[3];
        int n = read(STDIN_FILENO, buf, sizeof(buf));

        if (n > 0) {
            if (currentState == MENU) {
                if (buf[0] == 'w' || (buf[0] == '\033' && buf[2] == 'A')) {
                    menuSelection = (menuSelection - 1 + 3) % 3;
                }
                else if (buf[0] == 's' || (buf[0] == '\033' && buf[2] == 'B')) {
                    menuSelection = (menuSelection + 1) % 3;
                }
                else if (buf[0] == '\n' || buf[0] == ' ') {
                    if (menuSelection == 0) {
                        currentState = GAME;
                        resetGame();
                    } else if (menuSelection == 1) {
                        currentState = SETTINGS;
                    } else {
                        exit(0);
                    }
                }
            }
            else if (currentState == SETTINGS) {
                 if (buf[0] == 'w' || (buf[0] == '\033' && buf[2] == 'A')) {
                    settingsSelection = (settingsSelection - 1 + 3) % 3;
                }
                else if (buf[0] == 's' || (buf[0] == '\033' && buf[2] == 'B')) {
                    settingsSelection = (settingsSelection + 1) % 3;
                }
                else if (buf[0] == 'a' || (buf[0] == '\033' && buf[2] == 'D')) {
                    if(settingsSelection == 0) setSpeedIndex = (setSpeedIndex - 1 + 4) % 4;
                    if(settingsSelection == 1) setGhostCount = (setGhostCount > 0) ? setGhostCount - 1 : 4;
                }
                else if (buf[0] == 'd' || (buf[0] == '\033' && buf[2] == 'C')) {
                    if(settingsSelection == 0) setSpeedIndex = (setSpeedIndex + 1) % 4;
                    if(settingsSelection == 1) setGhostCount = (setGhostCount < 10) ? setGhostCount + 1 : 0;
                }
                else if (buf[0] == '\n' || buf[0] == ' ') {
                    if(settingsSelection == 2) currentState = MENU;
                }
            }
            else if (currentState == GAME) {
                if (buf[0] == 'q') currentState = MENU;
                else if (buf[0] == 'w') nextDir = {0, -1};
                else if (buf[0] == 's') nextDir = {0, 1};
                else if (buf[0] == 'a') nextDir = {-1, 0};
                else if (buf[0] == 'd') nextDir = {1, 0};
                else if (buf[0] == '\033' && n >= 3) {
                    switch(buf[2]) {
                        case 'A': nextDir = {0, -1}; break;
                        case 'B': nextDir = {0, 1}; break;
                        case 'C': nextDir = {1, 0}; break;
                        case 'D': nextDir = {-1, 0}; break;
                    }
                }
            }
            else if (currentState == GAME_OVER) {
                if (buf[0] == 'q') currentState = MENU;
                if (buf[0] == 'r') {
                    currentState = GAME;
                    resetGame();
                }
            }
        }
    }

    bool isWall(int x, int y) {
        if(x < 0 || x >= w || y < 0 || y >= h) return true;
        return map[y][x] == '#';
    }

    void updateGame() {
        if (isPowered) {
            powerTimer--;
            if(powerTimer <= 0) isPowered = false;
        }

        if (!isWall(player.x + nextDir.x, player.y + nextDir.y)) dir = nextDir;

        if (!isWall(player.x + dir.x, player.y + dir.y)) {
            player.x += dir.x;
            player.y += dir.y;
        }

        char cell = map[player.y][player.x];
        if (cell == '.') {
            map[player.y][player.x] = ' ';
            score += 10;
        } else if (cell == '*') {
            map[player.y][player.x] = ' ';
            score += 50;
            isPowered = true;
            powerTimer = POWER_DURATION;
        }

        for(auto& g : ghosts) {
            if ((g.dir.x == 0 && g.dir.y == 0) || isWall(g.pos.x + g.dir.x, g.pos.y + g.dir.y) || (rand() % 10 == 0)) {
                vector<Point> moves;
                if(!isWall(g.pos.x+1, g.pos.y)) moves.push_back({1, 0});
                if(!isWall(g.pos.x-1, g.pos.y)) moves.push_back({-1, 0});
                if(!isWall(g.pos.x, g.pos.y+1)) moves.push_back({0, 1});
                if(!isWall(g.pos.x, g.pos.y-1)) moves.push_back({0, -1});
                if(!moves.empty()) g.dir = moves[rand() % moves.size()];
            }
            g.pos.x += g.dir.x;
            g.pos.y += g.dir.y;

            if (g.pos.x == player.x && g.pos.y == player.y) {
                if (isPowered) {
                    score += 200;
                    g.pos = ghostSpawn; 
                } else {
                    currentState = GAME_OVER;
                }
            }
        }

        bool dotsLeft = false;
        for(const auto& row : map) if(row.find('.') != string::npos || row.find('*') != string::npos) dotsLeft = true;
        if(!dotsLeft) { 
            currentState = GAME_OVER; 
            win = true; 
        }
    }

    void cursorTo(string& buf, int r, int c) {
        buf += "\033[" + to_string(padTop + r) + ";" + to_string(padLeft + c) + "H";
    }

    void drawButton(string& buf, int r, string text, bool selected) {
        int width = w * 2;
        int textLen = text.length();
        int btnWidth = textLen + 4;
        int startCol = (width - btnWidth) / 2;
        
        cursorTo(buf, r, startCol);
        
        if (selected) {
            buf += string(BG_WHITE) + BLACK_TXT + "  " + text + "  " + RESET;
        } else {
            buf += string(BG_BLACK) + WHITE + "  " + text + "  " + RESET;
        }
    }

    void drawFrame(string& buf, int heightOffset = 0) {
        cursorTo(buf, 0, 0);
        buf += GRAY "╔";
        for(int i=0; i<w*2; i++) buf += "═";
        buf += "╗" RESET;

        for(int y=0; y<h + heightOffset; y++) {
            cursorTo(buf, y + 1, 0);
            buf += GRAY "║" RESET;
            cursorTo(buf, y + 1, (w*2) + 1);
            buf += GRAY "║" RESET;
        }

        cursorTo(buf, h + heightOffset + 1, 0);
        buf += GRAY "╚";
        for(int i=0; i<w*2; i++) buf += "═";
        buf += "╝" RESET;
    }

    void draw() {
        calculateLayout();
        string buffer = "\033[2J"; 

        if (currentState == MENU) {
            drawFrame(buffer);
            
            string title = "PAC-MAN";
            cursorTo(buffer, 4, (w*2 - title.length())/2);
            buffer += YELLOW + title + RESET;

            drawButton(buffer, 8, "START GAME", menuSelection == 0);
            drawButton(buffer, 11, "SETTINGS", menuSelection == 1);
            drawButton(buffer, 14, "EXIT", menuSelection == 2);
        } 
        else if (currentState == SETTINGS) {
            drawFrame(buffer);

            string title = "SETTINGS";
            cursorTo(buffer, 3, (w*2 - title.length())/2);
            buffer += BLUE + title + RESET;

            string speeds[] = {"SLOW", "NORMAL", "FAST", "INSANE"};
            string spd = "SPEED: < " + speeds[setSpeedIndex] + " >";
            drawButton(buffer, 7, spd, settingsSelection == 0);

            string gcount = "GHOSTS: < " + to_string(setGhostCount) + " >";
            drawButton(buffer, 10, gcount, settingsSelection == 1);

            drawButton(buffer, 14, "BACK", settingsSelection == 2);
        }
        else {
            drawFrame(buffer);

            for(int y=0; y<h; y++) {
                cursorTo(buffer, y + 1, 2);
                for(int x=0; x<w; x++) {
                    bool dynamic = false;
                    
                    if(x == player.x && y == player.y) {
                        string pChar = " O";
                        if(dir.x == 1)      pChar = " <";
                        else if(dir.x == -1) pChar = " >";
                        else if(dir.y == -1) pChar = " v";
                        else if(dir.y == 1)  pChar = " ^";
                        
                        buffer += YELLOW + pChar + RESET;
                        dynamic = true;
                    } 
                    else {
                        for(const auto& g : ghosts) {
                            if(g.pos.x == x && g.pos.y == y) {
                                if (isPowered) buffer += CYAN + GHOST_CHAR + RESET;
                                else buffer += RED + GHOST_CHAR + RESET;
                                dynamic = true;
                                break;
                            }
                        }
                    }

                    if(!dynamic) {
                        char c = map[y][x];
                        if(c == '#') buffer += BLUE + WALL_CHAR + RESET;
                        else if(c == '.') buffer += WHITE + DOT_CHAR + RESET;
                        else if(c == '*') buffer += GREEN + POWER_CHAR + RESET;
                        else buffer += EMPTY_CHAR;
                    }
                }
            }

            string sText = "SCORE: " + to_string(score);
            cursorTo(buffer, h + 2, (w*2 - sText.length())/2);
            buffer += WHITE + sText + RESET;

            if (currentState == GAME_OVER) {
                string msg = win ? "YOU WIN!" : "GAME OVER";
                string sub = "R - Retry | Q - Menu";
                
                int boxWidth = sub.length() + 4;
                int boxHeight = 5;
                int startY = (h - boxHeight) / 2;
                int startX = (w*2 - boxWidth) / 2;

                for(int i=0; i<boxHeight; i++) {
                    cursorTo(buffer, startY + i, startX);
                    buffer += BG_BLACK;
                    for(int j=0; j<boxWidth; j++) buffer += " ";
                    buffer += RESET;
                }

                cursorTo(buffer, startY + 1, (w*2 - msg.length()) / 2);
                buffer += string(win ? GREEN : RED) + BG_BLACK + msg + RESET;
                
                cursorTo(buffer, startY + 3, (w*2 - sub.length()) / 2);
                buffer += string(WHITE) + BG_BLACK + sub + RESET;
            }
        }

        cout << buffer << flush;
    }

    void run() {
        setupTerminal();
        while(true) {
            handleInput();
            if (currentState == GAME) updateGame();
            draw();
            usleep(gameSpeedDelay);
        }
    }
};

int main() {
    srand(time(0));
    PacmanGame game;
    game.run();
    return 0;
}

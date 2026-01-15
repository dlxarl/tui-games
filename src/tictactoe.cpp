#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <algorithm>

#define COLOR_RESET  "\033[0m"
#define COLOR_RED    "\033[1;31m"
#define COLOR_CYAN   "\033[1;36m"
#define COLOR_GREEN  "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_GRID   "\033[90m"
#define BG_SELECTED  "\033[47m"

struct TermConfig {
    struct termios orig_termios;
    TermConfig() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        std::cout << "\033[?1000h\033[?25l";
    }
    ~TermConfig() {
        std::cout << "\033[?1000l\033[?25h";
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
};

enum GameState { STATE_MENU, STATE_SETTINGS, STATE_GAME };
enum Difficulty { DIFF_EASY, DIFF_MEDIUM, DIFF_HARD, DIFF_IMPOSSIBLE };

class Game {
    char board[3][3];
    int selX, selY;
    char currentPlayer;
    
    GameState currentState;
    bool running;
    bool vsComputer;
    bool exitProgram;
    
    Difficulty difficulty;
    bool playerStarts; 
    int settingsRow;

    int menuSelection;

    int boardRow, boardCol; 
    std::string message;

public:
    Game() : selX(1), selY(1), currentPlayer('X'), running(false), 
             vsComputer(false), currentState(STATE_MENU), 
             menuSelection(0), exitProgram(false),
             difficulty(DIFF_MEDIUM), playerStarts(true), settingsRow(0) {
        std::srand(std::time(0));
    }

    void resetBoard() {
        for(int i=0; i<3; i++)
            for(int j=0; j<3; j++)
                board[i][j] = ' ';
        selX = 1; selY = 1;
        running = true;
        
        currentPlayer = playerStarts ? 'X' : 'O';
        
        if (currentPlayer == 'X') message = std::string(COLOR_RED) + "Player X's turn" + std::string(COLOR_RESET);
        else message = std::string(COLOR_CYAN) + "Bot O is thinking..." + std::string(COLOR_RESET);

        if (vsComputer && !playerStarts) {
            drawGame();
            usleep(500000);
            computerMove();
        }
    }

    void moveTo(int r, int c) {
        std::cout << "\033[" << (boardRow + r) << ";" << (boardCol + c) << "H";
    }

    void printCellLine(int cellR, int cellC, int lineIndex) {
        bool isSelected = (cellR == selX && cellC == selY && running && currentPlayer == 'X'); 
        if (vsComputer && currentPlayer == 'O') isSelected = false;

        if (isSelected) std::cout << BG_SELECTED;
        
        if (lineIndex == 1) { 
            char sym = board[cellR][cellC];
            std::cout << "   ";
            if (sym == 'X') std::cout << COLOR_RED << "X" << COLOR_RESET;
            else if (sym == 'O') std::cout << COLOR_CYAN << "O" << COLOR_RESET;
            else std::cout << " ";
            if (isSelected) std::cout << BG_SELECTED << "   " << COLOR_RESET;
            else std::cout << "   ";
        } else { 
            std::cout << "       ";
            if (isSelected) std::cout << COLOR_RESET;
        }
    }

    void drawMenu() {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        boardRow = (w.ws_row - 12) / 2;
        boardCol = (w.ws_col - 30) / 2;

        std::cout << "\033[2J"; 
        moveTo(0, 5); std::cout << COLOR_CYAN << "TIC-TAC-TOE" << COLOR_RESET;
        
        const char* opts[] = {"1 Player (vs Bot)", "2 Players (Local)", "Exit"};
        for(int i=0; i<3; i++) {
            moveTo(3 + i*2, 0);
            if (menuSelection == i) std::cout << COLOR_YELLOW << "> " << opts[i] << COLOR_RESET;
            else std::cout << "  " << opts[i];
        }

        moveTo(10, 0);
        std::cout << COLOR_GRID << "Use Arrows & Enter" << COLOR_RESET;
        std::cout << std::flush;
    }

    void drawSettings() {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        boardRow = (w.ws_row - 12) / 2;
        boardCol = (w.ws_col - 40) / 2;

        std::cout << "\033[2J";
        moveTo(0, 10); std::cout << COLOR_GREEN << "GAME SETTINGS" << COLOR_RESET;

        moveTo(3, 0);
        if (settingsRow == 0) std::cout << COLOR_YELLOW << "> Difficulty: " << COLOR_RESET;
        else std::cout << "  Difficulty: ";
        
        std::string dStr;
        switch(difficulty) {
            case DIFF_EASY: dStr = "Easy (Random)"; break;
            case DIFF_MEDIUM: dStr = "Medium (Balanced)"; break;
            case DIFF_HARD: dStr = "Hard (Smart)"; break;
            case DIFF_IMPOSSIBLE: dStr = "Impossible (Minimax)"; break;
        }
        if (settingsRow == 0) std::cout << "< " << COLOR_CYAN << dStr << COLOR_RESET << " >";
        else std::cout << dStr;

        moveTo(5, 0);
        if (settingsRow == 1) std::cout << COLOR_YELLOW << "> First Move: " << COLOR_RESET;
        else std::cout << "  First Move: ";
        
        std::string pStr = playerStarts ? "Player (X)" : "Bot (O)";
        if (settingsRow == 1) std::cout << "< " << COLOR_CYAN << pStr << COLOR_RESET << " >";
        else std::cout << pStr;

        moveTo(8, 10);
        if (settingsRow == 2) std::cout << BG_SELECTED << " [ START GAME ] " << COLOR_RESET;
        else std::cout << " [ START GAME ] ";

        moveTo(11, 0);
        std::cout << COLOR_GRID << "Arrows: Move/Change | Enter: Select" << COLOR_RESET;
        std::cout << std::flush;
    }

    void drawGame() {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        boardRow = (w.ws_row - 16) / 2;
        boardCol = (w.ws_col - 25) / 2;
        if (boardRow < 1) boardRow = 1;
        if (boardCol < 1) boardCol = 1;

        std::cout << "\033[2J"; 
        moveTo(-2, 6); std::cout << COLOR_CYAN << "TIC-TAC-TOE" << COLOR_RESET;
        moveTo(0, 0); std::cout << COLOR_GRID << "┌───────┬───────┬───────┐" << COLOR_RESET;
        
        for (int i = 0; i < 3; ++i) {
            int startRow = 1 + i * 4; 
            for (int line = 0; line < 3; line++) {
                moveTo(startRow + line, 0);
                std::cout << COLOR_GRID << "│" << COLOR_RESET; 
                for (int j = 0; j < 3; j++) {
                    printCellLine(i, j, line);
                    std::cout << COLOR_GRID << "│" << COLOR_RESET; 
                }
            }
            moveTo(startRow + 3, 0);
            std::cout << COLOR_GRID;
            if (i == 2) std::cout << "└───────┴───────┴───────┘";
            else        std::cout << "├───────┼───────┼───────┤";
            std::cout << COLOR_RESET;
        }

        moveTo(14, 0); 
        std::cout << "\033[" << (boardCol) << "G" << message;
        moveTo(16, -10);
        std::cout << "\033[" << (boardCol - 10) << "G" << COLOR_GRID << "[ARROWS] Move [ENTER] Select [Q] Menu" << COLOR_RESET;
        std::cout << std::flush;
    }

    char checkWinnerSim(char b[3][3]) {
        for (int i = 0; i < 3; i++) {
            if (b[i][0] != ' ' && b[i][0] == b[i][1] && b[i][1] == b[i][2]) return b[i][0];
            if (b[0][i] != ' ' && b[0][i] == b[1][i] && b[1][i] == b[2][i]) return b[0][i];
        }
        if (b[0][0] != ' ' && b[0][0] == b[1][1] && b[1][1] == b[2][2]) return b[0][0];
        if (b[0][2] != ' ' && b[0][2] == b[1][1] && b[1][1] == b[2][0]) return b[0][2];
        return 0;
    }

    bool isMovesLeft(char b[3][3]) {
        for(int i=0; i<3; i++) for(int j=0; j<3; j++) if(b[i][j] == ' ') return true;
        return false;
    }

    int minimax(char b[3][3], int depth, bool isMax) {
        char winner = checkWinnerSim(b);
        if (winner == 'O') return 10 - depth;
        if (winner == 'X') return -10 + depth;
        if (!isMovesLeft(b)) return 0;

        if (isMax) {
            int best = -1000;
            for(int i=0; i<3; i++) {
                for(int j=0; j<3; j++) {
                    if (b[i][j] == ' ') {
                        b[i][j] = 'O';
                        best = std::max(best, minimax(b, depth + 1, !isMax));
                        b[i][j] = ' ';
                    }
                }
            }
            return best;
        } else {
            int best = 1000;
            for(int i=0; i<3; i++) {
                for(int j=0; j<3; j++) {
                    if (b[i][j] == ' ') {
                        b[i][j] = 'X';
                        best = std::min(best, minimax(b, depth + 1, !isMax));
                        b[i][j] = ' ';
                    }
                }
            }
            return best;
        }
    }

    void computerMove() {
        if (!running) return;

        bool useSmartMove = false;
        int chance = std::rand() % 100;

        switch (difficulty) {
            case DIFF_EASY:       useSmartMove = false; break;
            case DIFF_MEDIUM:     useSmartMove = (chance < 50); break;
            case DIFF_HARD:       useSmartMove = (chance < 90); break;
            case DIFF_IMPOSSIBLE: useSmartMove = true; break;
        }

        int bestR = -1, bestC = -1;

        if (useSmartMove) {
            int bestVal = -1000;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (board[i][j] == ' ') {
                        board[i][j] = 'O';
                        int moveVal = minimax(board, 0, false);
                        board[i][j] = ' ';
                        if (moveVal > bestVal) {
                            bestR = i; bestC = j; bestVal = moveVal;
                        }
                    }
                }
            }
        } 
        
        if (!useSmartMove || bestR == -1) {
            std::vector<std::pair<int, int>> empty;
            for(int i=0; i<3; i++) for(int j=0; j<3; j++) if(board[i][j]==' ') empty.push_back({i,j});
            if(!empty.empty()) {
                int idx = std::rand() % empty.size();
                bestR = empty[idx].first;
                bestC = empty[idx].second;
            }
        }

        if (bestR != -1) {
            board[bestR][bestC] = 'O';
            handleGameEndCheck();
        }
    }

    void handleGameEndCheck() {
        char winner = checkWinnerSim(board);
        if (winner != 0) {
            if (vsComputer && winner == 'O') message = std::string(COLOR_CYAN) + "Bot Wins! Press key." + std::string(COLOR_RESET);
            else if (winner == 'X') message = std::string(COLOR_RED) + "Player X Wins! Press key." + std::string(COLOR_RESET);
            else message = std::string(COLOR_CYAN) + "Player O Wins! Press key." + std::string(COLOR_RESET);
            running = false;
        } else if (!isMovesLeft(board)) {
            message = "It's a Draw! Press key.";
            running = false;
        } else {
            currentPlayer = (currentPlayer == 'X' ? 'O' : 'X');
            if (currentPlayer == 'X') message = std::string(COLOR_RED) + "Player X's turn" + std::string(COLOR_RESET);
            else message = std::string(COLOR_CYAN) + "Bot O is thinking..." + std::string(COLOR_RESET);
        }
    }

    void makeMove() {
        if (board[selX][selY] == ' ') {
            board[selX][selY] = currentPlayer;
            handleGameEndCheck();
            if (running && vsComputer && currentPlayer == 'O') {
                drawGame();
                usleep(300000); 
                computerMove();
            }
        }
    }

    void handleMouse(int mx, int my) {
        if (currentState != STATE_GAME) return;
        int startX = boardCol + 1; 
        int startY = boardRow + 1; 
        if (mx < startX || mx > startX + 24 || my < startY || my > startY + 11) return;
        int relX = mx - startX; 
        int relY = my - startY; 
        int gridCol = relX / 8; 
        int gridRow = relY / 4;
        if (gridRow >= 0 && gridRow < 3 && gridCol >= 0 && gridCol < 3) {
            selX = gridRow; selY = gridCol;
            makeMove();
        }
    }

    void run() {
        char buf[6];
        while (!exitProgram) {
            if (currentState == STATE_MENU) {
                drawMenu();
                if (read(STDIN_FILENO, buf, 1) > 0) {
                    if (buf[0] == '\033') {
                        read(STDIN_FILENO, buf+1, 2);
                        if (buf[1] == '[') {
                            if (buf[2] == 'A') menuSelection = (menuSelection - 1 + 3) % 3;
                            else if (buf[2] == 'B') menuSelection = (menuSelection + 1) % 3;
                        }
                    } else if (buf[0] == '\n' || buf[0] == '\r' || buf[0] == ' ') {
                        if (menuSelection == 0) { 
                            vsComputer = true; 
                            currentState = STATE_SETTINGS;
                        } else if (menuSelection == 1) { 
                            vsComputer = false; 
                            currentState = STATE_GAME; 
                            resetBoard(); 
                        } else if (menuSelection == 2) { 
                            exitProgram = true; 
                        }
                    } else if (buf[0] == 'q') exitProgram = true;
                }
            } 
            else if (currentState == STATE_SETTINGS) {
                drawSettings();
                if (read(STDIN_FILENO, buf, 1) > 0) {
                    if (buf[0] == '\033') {
                        read(STDIN_FILENO, buf+1, 2);
                        if (buf[1] == '[') {
                            if (buf[2] == 'A') settingsRow = (settingsRow - 1 + 3) % 3;
                            else if (buf[2] == 'B') settingsRow = (settingsRow + 1) % 3;
                            else if (buf[2] == 'C') {
                                if(settingsRow == 0) difficulty = (Difficulty)((difficulty + 1) % 4);
                                if(settingsRow == 1) playerStarts = !playerStarts;
                            }
                            else if (buf[2] == 'D') {
                                if(settingsRow == 0) difficulty = (Difficulty)((difficulty - 1 + 4) % 4);
                                if(settingsRow == 1) playerStarts = !playerStarts;
                            }
                        }
                    } else if (buf[0] == '\n' || buf[0] == '\r' || buf[0] == ' ') {
                        if (settingsRow == 2) {
                            currentState = STATE_GAME;
                            resetBoard();
                        }
                    } else if (buf[0] == 'q') currentState = STATE_MENU;
                }
            }
            else if (currentState == STATE_GAME) {
                drawGame();
                if (read(STDIN_FILENO, buf, 1) > 0) {
                    if (!running) { currentState = STATE_MENU; continue; }
                    if (buf[0] == 'q') { currentState = STATE_MENU; continue; }
                    if (vsComputer && currentPlayer == 'O') continue;

                    if (buf[0] == '\n' || buf[0] == '\r' || buf[0] == ' ') {
                        makeMove();
                    } else if (buf[0] == '\033') {
                        read(STDIN_FILENO, buf+1, 2); 
                        if (buf[1] == '[') {
                            if (buf[2] == 'A') selX = (selX - 1 + 3) % 3; 
                            else if (buf[2] == 'B') selX = (selX + 1) % 3;
                            else if (buf[2] == 'C') selY = (selY + 1) % 3;
                            else if (buf[2] == 'D') selY = (selY - 1 + 3) % 3;
                            else if (buf[2] == 'M') {
                                read(STDIN_FILENO, buf+3, 3);
                                handleMouse((unsigned char)buf[4]-32, (unsigned char)buf[5]-32);
                            }
                        }
                    }
                }
            }
        }
    }
};

int main() {
    TermConfig tc;
    Game game;
    game.run();
    return 0;
}

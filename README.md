# C++ Terminal Minigames

![Launcher](https://i.postimg.cc/zGRLg0WW/Screenshot-2026-01-15-at-21-49-31.png)

A collection of classic terminal games written in modern C++17 with an ncurses UI. Includes a menu-based launcher and individual binaries for each game.

## Overview
- Games: 2048, Minesweeper, Snake, Sudoku, Tic‑Tac‑Toe (TUI)
- UI: ncurses-based launcher with favorites, recents, and search
- Platform: macOS (tested); should work on Linux with ncurses
- Standard: C++17

## Repository Layout
- `launcher.cpp`: ncurses menu application to browse and run games
- `src/`: game sources (`2048.cpp`, `minesweeper.cpp`, `snake.cpp`, `sudoku.cpp`, `tictactoe.cpp`)
- `exec/`: auto-created folder for compiled game binaries
- `dev/`: development playgrounds and per-game subprojects
- `.gitignore`: ignores `dev/`, `exec/`, highscore files, and binaries

## Prerequisites
- C++ compiler: `g++` with C++17 support
- ncurses library:
	- macOS (Homebrew): `brew install ncurses`
	- Linux: use your distro package manager (e.g., `sudo apt-get install libncurses5-dev libncursesw5-dev`)

The provided Makefile on macOS assumes Homebrew paths:
- Includes: `/opt/homebrew/opt/ncurses/include`
- Libs: `/opt/homebrew/opt/ncurses/lib`

If your ncurses is installed elsewhere, adjust `INCLUDES` and `LIBDIRS` in the top-level `Makefile` accordingly.

## Build
From the repository root:

```bash
# Build launcher + all games (creates exec/)
make

# Or explicitly
make all

# Clean binaries
make clean
```

This compiles:
- `launcher` (menu app)
- Game binaries into `exec/`: `2048`, `minesweeper`, `snake`, `sudoku`, `tictactoe`

Compiler and linker flags (from Makefile):
- `-Wall -std=c++17`
- Links: `-lncurses`

## Run
You can use the launcher or run games directly.

Launcher:
```bash
./launcher
```

Run a specific game binary:
```bash
./exec/snake
./exec/minesweeper
./exec/2048
./exec/sudoku
./exec/tictactoe
```

## Launcher Controls
- Navigation: Arrow keys or `W/A/S/D`
- Enter: launch selected game
- `F`: toggle favorite for selected game
- `/`: search mode (type to filter; Enter to exit, Esc to clear)
- `TAB`: switch category (All / Favorites / Recents)
- `V`: toggle list vs block view
- `Q`: quit launcher

Favorites and recent selections persist in `launcher.dat` at the project root.

## Game Descriptions
### 2048
![2048](https://i.postimg.cc/4xhK9Bt7/Screenshot-2026-01-15-at-21-49-49.png)
Slide numbered tiles on a 4×4 grid to combine matching values and create higher numbers. Strategic movement avoids blocking the board while aiming for the 2048 tile. Easy to learn, challenging to master.

### Minesweeper
![Minesweeper](https://i.postimg.cc/XYyZCQFC/Screenshot-2026-01-15-at-21-50-49.png)
Reveal cells on a grid without detonating hidden mines. Numbers indicate how many mines touch a cell, enabling logical deduction. A classic puzzle of inference and risk management.

### Snake
![Snake](https://i.postimg.cc/NjryHpTL/Screenshot-2026-01-15-at-21-51-59.png)
Guide the snake to eat food and grow longer while avoiding collisions with walls and your own body. Plan routes and timing to survive as the speed and length increase.

### Sudoku
![Sudoku](https://i.postimg.cc/T3WyDCmb/Screenshot-2026-01-15-at-21-51-35.png)
Fill the 9×9 grid so each row, column, and 3×3 box contains the digits 1–9 exactly once. Requires logic and pattern recognition; no guessing needed for well-formed puzzles.

### Tic‑Tac‑Toe
![Tic‑Tac‑Toe](https://i.postimg.cc/3xDyvnpD/Screenshot-2026-01-15-at-21-50-01.png)
Place X or O on a 3×3 board and aim to align three in a row horizontally, vertically, or diagonally. Simple rules with opportunities for strategic play and forced draws.

## Data & Files
- Highscores: `highscore.txt` and `highscore_2048.txt` (ignored by git)
- Launcher state: `launcher.dat` (favorites/recents)
- Binaries: `exec/` (ignored by git)
- Dev playground: `dev/` (ignored by git)

## Troubleshooting
- ncurses not found:
	- Verify install: `brew info ncurses`
	- Export paths if needed:
		```bash
		export CPATH=/opt/homebrew/opt/ncurses/include
		export LIBRARY_PATH=/opt/homebrew/opt/ncurses/lib
		```
	- Or update `INCLUDES` / `LIBDIRS` in `Makefile`.

- garbled characters or missing borders:
	- Ensure a UTF‑8 locale is active (launcher calls `setlocale(LC_ALL, "")`).
	- Use a terminal that supports line-drawing characters.

## Notes
- The launcher scans `./exec` for non-hidden files and runs them.
- `.gitignore` excludes typical compiled artifacts and platform files.
- Linux users may need to replace Homebrew paths with system ncurses paths.

## License
MIT License
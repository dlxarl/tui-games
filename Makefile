CXX       = g++
CXXFLAGS  = -Wall -std=c++17
LDFLAGS   = -lncurses
INCLUDES  = -I/opt/homebrew/opt/ncurses/include
LIBDIRS   = -L/opt/homebrew/opt/ncurses/lib

SRC_DIR   = src
EXEC_DIR  = exec
TARGET    = launcher

GAME_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
GAMES     := $(basename $(notdir $(GAME_SRCS)))
GAME_BINS := $(addprefix $(EXEC_DIR)/,$(GAMES))

.PHONY: all setup clean games

all: setup $(TARGET) games

$(TARGET): launcher.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LIBDIRS) $(LDFLAGS)

games: $(GAME_BINS)

$(EXEC_DIR)/%: $(SRC_DIR)/%.cpp | setup
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LIBDIRS) $(LDFLAGS)

setup:
	mkdir -p $(EXEC_DIR)

clean:
	rm -f $(TARGET) $(GAME_BINS)
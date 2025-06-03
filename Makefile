# Detectar si se está invocando clean sin compilar
ifeq ($(filter clean,$(MAKECMDGOALS)),)
	ifndef MODE
		MODE := debug
	endif
endif

TARGET = server

SRC_DIR = src
BUILD_DIR = build

CXX = g++
CXXFLAGS_DEBUG = -std=c++20 -g -Wall -Wextra -fvisibility=hidden -fno-inline
CXXFLAGS_PROD  = -std=c++20 -O3 -Wall -Wextra -fvisibility=hidden -fno-inline
LDFLAGS_DEBUG  = -lpthread -lsqlite3 -lcrypto
LDFLAGS_PROD   = -lpthread -lsqlite3 -lcrypto -s 
ARCHFLAGS      = -march=native -flto=2 -msse2

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Selección de flags
ifeq ($(MODE),debug)
	CXXFLAGS := $(CXXFLAGS_DEBUG)
	LDFLAGS  := $(LDFLAGS_DEBUG)
else ifeq ($(MODE),prod)
	CXXFLAGS := $(CXXFLAGS_PROD)
	LDFLAGS  := $(LDFLAGS_PROD)
else ifdef MODE
	$(error MODE inválido: $(MODE). Usa 'debug' o 'prod')
endif

# Default
all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

# Alias
debug:
	@$(MAKE) MODE=debug all

prod:
	@$(MAKE) MODE=prod all

# Compilación
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) -c $< -o $@

# Limpiar solo
clean:
	rm -rf $(BUILD_DIR)

# Limpiar y compilar según MODE
clean_and:
	@$(MAKE) clean
	@if [ -n "$(MODE)" ]; then \
		$(MAKE) MODE=$(MODE); \
	fi

.PHONY: all clean debug prod clean_and
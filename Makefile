TARGET = web_atlantis

SRC_DIR = src
BUILD_DIR = build

CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall
LDFLAGS = -lpthread -lsqlite3 -lcrypto
ARCHFLAGS = -march=native -flto=2 -msse2

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(ARCHFLAGS) -c $< -o $@

# Limpiar archivos generados
clean:
	rm -rf $(BUILD_DIR)

# Recompilar todo desde cero
rebuild: clean all

# Evitar conflictos con nombres de archivos
.PHONY: all clean rebuild
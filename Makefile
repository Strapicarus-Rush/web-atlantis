# Detectar si se está invocando clean sin compilar
ifeq ($(filter clean,$(MAKECMDGOALS)),)
	ifndef MODE
		MODE = debug
	endif
endif

TARGET := web-atlantis

SRC_DIR := src
BUILD_DIR := build
DATA_DIR := data
CONFIG_DIR := config
LOG_DIR := log
PUBLIC_DIR := public

CXX := g++
AS := as
ASFLAGS :=

ARCHFLAGS := -march=native -flto=2

# Flags de compilación
# Flags de arquitectura comunes a todos los perfiles
CXXFLAGS_DEBUG := -std=c++20 -O0 -g -Wall -Wextra -fno-inline -MMD -MP
CXXFLAGS_PROD  := -std=c++20 -O3 -Wall -Wextra -fvisibility=hidden -MMD -MP

# Flags de linker
LDFLAGS_DEBUG := -lpthread -lsqlite3 -lcrypto
LDFLAGS_PROD  := -lpthread -lsqlite3 -lcrypto -s

ASM_SOURCES := $(wildcard $(SRC_DIR)/*.s)
CPP_SOURCES := $(wildcard $(SRC_DIR)/*.cpp)

SOURCES := $(CPP_SOURCES) $(ASM_SOURCES)

CPP_OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

OBJECTS := $(CPP_OBJECTS) $(ASM_OBJECTS)

ifneq ($(filter-out all info deps clean,$(MAKECMDGOALS)),)
  -include $(CPP_OBJECTS:.o=.d)
endif

# Install paths
BINDIR := /usr/local/bin
DATADIR := $(HOME)/.pccssystems/web-atlantis


# Selección de flags
ifeq ($(MODE), debug)
	CXXFLAGS := $(CXXFLAGS_DEBUG)
	LDFLAGS  := $(LDFLAGS_DEBUG)
	DEFINES  := -DDEV
else ifeq ($(MODE),prod)
	CXXFLAGS := $(CXXFLAGS_PROD)
	LDFLAGS  := $(LDFLAGS_PROD)
else ifdef MODE
	$(error MODE inválido: $(MODE). Usa 'debug' o 'prod')
endif

# Default
all: info deps

path: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

info:
	@echo ""
	@echo "Ejecuta 'make [opción]'"
	@echo "-Opciones: "
	@echo "	dev	(Compila modo dev)"
	@echo "	prod (Compila y copia public a $(DATADIR))"
	@echo "	clean (Elimina directorios data, log, config en el directorio del proyecto)"
	@echo "	clean_and (Elimina directorios y compila en modo debug por defecto)"
	@echo "	install (Copia el ejecutable a $(BINDIR) y public a $(DATADIR))"
	@echo ""

deps:
	@echo "Requiere las siguientes librerías instaladas:"
	@echo "   - libssl-dev"
	@echo "   - libsqlite3-dev"
	@echo "   - nlohmann json"
	@echo "   - libpthread"
	@echo "   - libc6-dev y libstdc++-dev"
	@echo "Si la compilación falla, instala los paquetes."
	@echo ""

# Alias
debug:
	@$(MAKE) MODE=debug clean path

prod:
	@$(MAKE) MODE=prod path

public:
	mkdir -p $(DATADIR)
	cp -r $(PUBLIC_DIR)/* $(DATADIR)/$(PUBLIC_DIR)

install:
# 	@echo "Se Requiere intalar con sudo"
# 	sudo mkdir -p $(BINDIR)
	install -Dm755 $(BUILD_DIR)/$(TARGET) $(BINDIR)/$(TARGET)
	mkdir -p $(DATADIR)
	cp -r $(PUBLIC_DIR)/* $(DATADIR)/$(PUBLIC_DIR)

# Compilación
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) $(ARCHFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	$(AS) $(ASFLAGS) $< -o $@

# Limpiar
clean:
	rm -rf $(BUILD_DIR) $(DATA_DIR) $(CONFIG_DIR) $(LOG_DIR)

# Limpiar y compilar según MODE
clean_and:
	@$(MAKE) clean
	@if [ -n "$(MODE)" ]; then \
		$(MAKE) MODE=$(MODE); \
	fi

.PHONY: all clean debug prod clean_and deps info install path public

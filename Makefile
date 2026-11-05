CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS := -Iinclude
LDFLAGS  :=

# make SANITIZE=1 ... compila com AddressSanitizer + UBSan
ifeq ($(SANITIZE),1)
    CXXFLAGS += -fsanitize=address,undefined -g -fno-omit-frame-pointer
    LDFLAGS  += -fsanitize=address,undefined
endif

BUILD   := build
HEADERS := $(wildcard include/*.hpp)
DEMO    := $(BUILD)/demo
TESTES  := $(BUILD)/testes

.PHONY: all run test clean

all: $(DEMO) $(TESTES)

$(DEMO): src/main.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS)

$(TESTES): tests/test_arvore_rubro_negra.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS)

$(BUILD):
	mkdir -p $(BUILD)

run: $(DEMO)
	./$(DEMO)

test: $(TESTES)
	./$(TESTES)

clean:
	rm -rf $(BUILD)

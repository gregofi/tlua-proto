CXX=g++
LD=g++
CXXFLAGS=-Wall -pedantic -std=c++23
CXXTESTFLAGS=-lCatch2Main -lCatch2

SRC_DIR=src
OBJ_DIR=build
BIN=tlua

SOURCES=$(wildcard $(SRC_DIR)/*.cpp)
OBJECT_FILES=$(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS=$(OBJECT_FILES:.o=.d)

TEST_DIR=test
TEST_BUILD_DIR=build/tests

TEST_SOURCES=$(wildcard $(TEST_DIR)/*.cpp)
TEST_BINS=$(TEST_SOURCES:$(TEST_DIR)/%.cpp=$(TEST_BUILD_DIR)/%)

all: $(BIN)

# linking
$(BIN): $(OBJECT_FILES)
	$(LD) $(OBJECT_FILES) -o $@

# compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
# create build directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

-include $(DEPS)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN) $(TEST_BUILD_DIR)

.PHONY: run
run: $(BIN)
	./$(BIN)

# == tests ==
$(TEST_BUILD_DIR):
	mkdir -p $(TEST_BUILD_DIR)

lexer_test_OBJS=$(OBJ_DIR)/lexer.o
parser_test_OBJS=$(OBJ_DIR)/parser.o $(OBJ_DIR)/lexer.o

.SECONDEXPANSION:
$(TEST_BUILD_DIR)/%: $(TEST_DIR)/%.cpp $$(%_OBJS) | $(TEST_BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ $(CXXTESTFLAGS) -o $@

.PHONY: test
test: $(TEST_BINS)
	@for t in $(TEST_BINS); do $$t; done

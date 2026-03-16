
.PHONY: all test clean

BUILD_DIR=build
TEST_NAME=Hal_test

all: test

# Explicit compilation command, for simplicity
# Function and data sections are required to avoid linking to anything
# that is not explicitly required in tests .

test:
	mkdir -p $(BUILD_DIR)
	gcc -ffunction-sections -fdata-sections -o $(BUILD_DIR)/$(TEST_NAME) \
		Hal_test_main.c \
		../../src/Hal/Hal.c \
		../unity/unity.c \
		-I../unity \
		-I../../src/Hal \
		-I../../src/SamV71Core \
		-I../../src/BootHelper \
		-I../../arm-bsp/src/ \
		-I../../src/Mocks \
		-I../mocks \
		-DN7S_TARGET_SAMV71Q21 \
		-Wl,--gc-sections
	./$(BUILD_DIR)/$(TEST_NAME)

clean:
	rm -rf $(BUILD_DIR)


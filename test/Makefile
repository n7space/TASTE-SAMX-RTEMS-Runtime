.PHONY: all test clean

# Hardcoded list of subfolders to test
SUBFOLDERS := Hal

all: test

test:
	@for d in $(SUBFOLDERS); do \
		echo "=== Running tests in $$d ==="; \
		$(MAKE) -C $$d test || exit $$?; \
	done

clean:
	@for d in $(SUBFOLDERS); do \
		echo "=== Cleaning $$d ==="; \
		$(MAKE) -C $$d clean || exit $$?; \
	done


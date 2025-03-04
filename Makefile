.PHONY: build run clean local

build:
	@echo "Building judge-compile..."
	./scripts/build.sh

run:
	@echo "Running judge-compile..."
	./scripts/run.sh

clean:
	@echo "Cleaning judge-compile..."
	./scripts/clean.sh

local:
	@echo "Running judge-compile locally..."
	./scripts/local.sh
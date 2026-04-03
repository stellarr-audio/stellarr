.PHONY: setup build build-ui build-cpp test run clean

.DEFAULT_GOAL := build

setup:
	cd ui && npm install

build-ui: setup
	cd ui && npm run build

build-cpp:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build

build: build-ui build-cpp

test: build
	ctest --test-dir build --output-on-failure

run: build
	open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app

clean:
	rm -rf build ui/dist ui/node_modules

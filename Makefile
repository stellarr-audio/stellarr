.PHONY: setup build build-ui build-cpp release release-cpp run run-release test docs clean

.DEFAULT_GOAL := build

setup:
	cd ui && npm install

build-ui: setup
	cd ui && npm run build

build-cpp:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build -j

build: build-ui build-cpp

release-cpp:
	cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
	cmake --build build-release --config Release -j

release: build-ui release-cpp
	@echo "Release build: build-release/Stellarr_artefacts/Release/Standalone/Stellarr.app"

run-release: release
	open build-release/Stellarr_artefacts/Release/Standalone/Stellarr.app

test: build
	ctest --test-dir build --output-on-failure

run: build
	open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app

docs:
	npx serve docs/manual -p 3001

clean:
	rm -rf build build-release ui/dist ui/node_modules

.PHONY: setup dev dev-ui dev-cpp debug debug-cpp release release-cpp run run-debug run-release test docs clean

.DEFAULT_GOAL := dev

setup:
	cd ui && npm install

dev-ui: setup
	cd ui && npm run build

dev-cpp:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build -j4

dev: dev-ui dev-cpp

debug-cpp:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build -j4

debug: dev-ui debug-cpp

release-cpp:
	cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
	cmake --build build-release --config Release -j4

release: dev-ui release-cpp

run: dev
	open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app

run-debug: debug
	open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app

run-release: release
	open build-release/Stellarr_artefacts/Release/Standalone/Stellarr.app

test: debug
	ctest --test-dir build --output-on-failure

docs:
	npx serve docs/manual -p 3001

clean:
	rm -rf build build-release ui/dist ui/node_modules

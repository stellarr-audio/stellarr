.PHONY: setup dev dev-ui dev-cpp debug debug-cpp release release-cpp run run-debug run-release test docs clean clear-cache

.DEFAULT_GOAL := dev

# Load env vars from ui/.env if it exists (same file Vite uses)
-include ui/.env
export

SENTRY_CMAKE_FLAG := $(if $(VITE_SENTRY_DSN),-DSTELLARR_SENTRY_DSN="$(VITE_SENTRY_DSN)",)

setup:
	cd ui && npm install

clear-cache:
	rm -rf ~/Library/Caches/com.Stellarr.Stellarr ~/Library/WebKit/com.Stellarr.Stellarr

dev-ui: setup clear-cache
	cd ui && rm -rf dist && npm run build

dev-cpp:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(SENTRY_CMAKE_FLAG)
	cmake --build build -j4

dev: dev-ui dev-cpp

debug-cpp:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(SENTRY_CMAKE_FLAG)
	cmake --build build -j4

debug: dev-ui debug-cpp

release-cpp:
	cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF $(SENTRY_CMAKE_FLAG)
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

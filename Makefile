.PHONY: setup dev dev-ui dev-cpp debug debug-cpp release release-cpp run run-debug run-release run-ui open test docs web clean clear-cache screenshots regen-sparkle-keys-prod regen-sparkle-keys-dev dev-updater-serve

.DEFAULT_GOAL := dev

# Load env vars from ui/.env if it exists (same file Vite uses)
-include ui/.env
export

SENTRY_CMAKE_FLAG := $(if $(VITE_SENTRY_DSN),-DSTELLARR_SENTRY_DSN="$(VITE_SENTRY_DSN)",)

STELLARR_FLAVOUR ?= prod
FLAVOUR_CMAKE_FLAG := -DSTELLARR_FLAVOUR=$(STELLARR_FLAVOUR)

setup:
	cd ui && npm install

clear-cache:
	rm -rf ~/Library/Caches/com.Stellarr.Stellarr ~/Library/WebKit/com.Stellarr.Stellarr

dev-ui: setup clear-cache
	cd ui && rm -rf dist && npm run build

dev-cpp:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(SENTRY_CMAKE_FLAG) $(FLAVOUR_CMAKE_FLAG)
	cmake --build build -j4

dev: dev-ui dev-cpp

debug-cpp:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(SENTRY_CMAKE_FLAG) $(FLAVOUR_CMAKE_FLAG)
	cmake --build build -j4

debug: dev-ui debug-cpp

release-cpp:
	cmake -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF $(SENTRY_CMAKE_FLAG) $(FLAVOUR_CMAKE_FLAG)
	cmake --build build-release --config Release -j4

release: dev-ui release-cpp

run: dev
	open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app

run-ui: dev-ui
	open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app

run-debug: debug
	open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app

run-release: release
	open build-release/Stellarr_artefacts/Release/Standalone/Stellarr.app

# Launch the existing debug app binary without rebuilding.
open:
	open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app

test: debug
	ctest --test-dir build --output-on-failure

screenshots:
	./scripts/screenshots.sh

docs:
	cd web && npm install --silent && npm run dev

# Alias for `make docs` — the Astro site covers both landing page and manual.
web: docs

clean:
	rm -rf build build-release ui/dist ui/node_modules web/dist web/node_modules web/.astro

# Regenerate Sparkle EdDSA signing keys.
#
# Each target:
#   1. Generates a new keypair in the macOS Keychain under the named account
#      (destroys the old private key for that flavour — the old public key in
#      CMakeLists.txt becomes obsolete the moment you commit the new one).
#   2. Exports the new private key to a temp file, uploads it as a GitHub
#      Actions secret, then shreds the temp file.
#   3. For dev: also writes the private key to ~/.stellarr/dev-updater/priv
#      (0600) so the local test harness can sign a fake update.
#   4. Prints the new public key. You MUST paste it into the matching
#      STELLARR_SUPUBLIC_EDKEY_PROD or STELLARR_SUPUBLIC_EDKEY_DEV value in
#      CMakeLists.txt and commit.
#
# Requires: gh authenticated with write access to the repo; the Sparkle
# framework fetched (`cmake -B build-prod` once has been run).
#
# WARNING: Rotating the prod key makes every already-installed Stellarr stop
# trusting future updates until users manually install a build carrying the
# new public key. Do not rotate prod on a whim.

SPARKLE_KEYGEN := ./build-prod/_deps/sparkle-src/bin/generate_keys
SPARKLE_GEN_GUARD := @if [ ! -x "$(SPARKLE_KEYGEN)" ]; then \
		echo "Sparkle generate_keys not found. Run 'cmake -B build-prod' first so Sparkle is fetched."; \
		exit 1; \
	fi

regen-sparkle-keys-prod:
	$(SPARKLE_GEN_GUARD)
	@echo "WARNING: rotating the PROD Sparkle key."
	@echo "Every existing install will stop trusting future updates signed by this key."
	@read -p "Continue? [y/N] " ans && [ "$$ans" = "y" ] || { echo "aborted"; exit 1; }
	@rm -rf /tmp/stellarr-prod-keygen && mkdir /tmp/stellarr-prod-keygen
	@security delete-generic-password -a stellarr-prod -s "https://sparkle-project.org" 2>/dev/null || true
	@$(SPARKLE_KEYGEN) --account stellarr-prod | grep -E "^ +<string>" | sed -E 's/.*<string>(.*)<\/string>.*/Prod pub key: \1/'
	@$(SPARKLE_KEYGEN) --account stellarr-prod -x /tmp/stellarr-prod-keygen/priv >/dev/null
	@gh secret set SPARKLE_PRIVATE_KEY_PROD < /tmp/stellarr-prod-keygen/priv
	@rm -P /tmp/stellarr-prod-keygen/priv && rmdir /tmp/stellarr-prod-keygen
	@echo "Prod secret uploaded. Paste the new pub key into STELLARR_SUPUBLIC_EDKEY_PROD in CMakeLists.txt."

# Serve a local Sparkle appcast for testing the update flow. Requires
# regen-sparkle-keys-dev to have been run once, and a built DMG to
# advertise as the "new" version.
#
# Usage:
#   make dev-updater-serve DMG=path/to/Stellarr-Dev-v0.99.1.dmg VERSION=0.99.1
dev-updater-serve:
	@if [ -z "$(DMG)" ] || [ -z "$(VERSION)" ]; then \
		echo "Usage: make dev-updater-serve DMG=<path> VERSION=<x.y.z>"; \
		exit 1; \
	fi
	@./scripts/dev-updater-serve.ts --dmg "$(DMG)" --version "$(VERSION)"

regen-sparkle-keys-dev:
	$(SPARKLE_GEN_GUARD)
	@mkdir -p $(HOME)/.stellarr/dev-updater
	@rm -rf /tmp/stellarr-dev-keygen && mkdir /tmp/stellarr-dev-keygen
	@security delete-generic-password -a stellarr-dev -s "https://sparkle-project.org" 2>/dev/null || true
	@$(SPARKLE_KEYGEN) --account stellarr-dev | grep -E "^ +<string>" | sed -E 's/.*<string>(.*)<\/string>.*/Dev pub key: \1/'
	@$(SPARKLE_KEYGEN) --account stellarr-dev -x /tmp/stellarr-dev-keygen/priv >/dev/null
	@gh secret set SPARKLE_PRIVATE_KEY_DEV < /tmp/stellarr-dev-keygen/priv
	@install -m 600 /tmp/stellarr-dev-keygen/priv $(HOME)/.stellarr/dev-updater/priv
	@rm -P /tmp/stellarr-dev-keygen/priv && rmdir /tmp/stellarr-dev-keygen
	@echo "Dev secret uploaded. Local priv updated at ~/.stellarr/dev-updater/priv."
	@echo "Paste the new pub key into STELLARR_SUPUBLIC_EDKEY_DEV in CMakeLists.txt."

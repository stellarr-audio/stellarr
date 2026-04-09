#!/usr/bin/env bash
set -euo pipefail

CONFIG="${1:-screenshots.json}"
APP="build/Stellarr_artefacts/Debug/Standalone/Stellarr.app"
SIGNAL_FILE="/tmp/stellarr-screenshot-ready"
TIMEOUT=30

if [ ! -f "$CONFIG" ]; then
  echo "Config file not found: $CONFIG"
  echo "Copy screenshots.example.json to screenshots.json and edit paths for your machine."
  exit 1
fi

if [ ! -d "$APP" ]; then
  echo "App not found at $APP — run 'make dev' first."
  exit 1
fi

count=$(python3 -c "import json; print(len(json.load(open('$CONFIG'))))")

for i in $(seq 0 $((count - 1))); do
  name=$(python3 -c "import json; print(json.load(open('$CONFIG'))[$i]['name'])")
  output=$(python3 -c "import json; print(json.load(open('$CONFIG'))[$i]['output'])")

  echo "[$name] Launching app..."
  rm -f "$SIGNAL_FILE"

  # Launch app in background with screenshot args
  open -a "$APP" --args --screenshot "$(pwd)/$CONFIG" "$i" &
  APP_PID=$!

  # Wait for ready signal
  elapsed=0
  while [ ! -f "$SIGNAL_FILE" ] && [ $elapsed -lt $TIMEOUT ]; do
    sleep 1
    elapsed=$((elapsed + 1))
  done

  if [ ! -f "$SIGNAL_FILE" ]; then
    echo "[$name] Timed out waiting for app to be ready."
    kill "$APP_PID" 2>/dev/null || true
    continue
  fi

  sleep 1  # Extra settle time for rendering

  # Get window ID and capture
  WINDOW_ID=$(osascript -e 'tell application "Stellarr" to id of window 1' 2>/dev/null || true)

  if [ -z "$WINDOW_ID" ]; then
    echo "[$name] Could not get window ID, trying by name..."
    WINDOW_ID=$(osascript -e 'tell application "System Events" to tell process "Stellarr" to get id of window 1' 2>/dev/null || true)
  fi

  if [ -n "$WINDOW_ID" ]; then
    screencapture -l "$WINDOW_ID" "$output"
    echo "[$name] Saved to $output"
  else
    echo "[$name] Could not capture window — falling back to screen capture"
    screencapture -w "$output"
  fi

  # Clean up
  rm -f "$SIGNAL_FILE"
  osascript -e 'tell application "Stellarr" to quit' 2>/dev/null || kill "$APP_PID" 2>/dev/null || true
  sleep 2  # Wait for app to fully exit
done

echo "Done."

#!/usr/bin/env bash
set -euo pipefail

CONFIG="${1:-screenshots.json}"
APP="$(pwd)/build/Stellarr_artefacts/Debug/Standalone/Stellarr.app"
SIGNAL_FILE="/tmp/stellarr-screenshot-ready"
CONFIG_FILE="/tmp/stellarr-screenshot-config.json"
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

count=$(python3 -c "
import json, sys
try:
    data = json.load(open('$CONFIG'))
    if not isinstance(data, list):
        print('Error: $CONFIG must be a JSON array.', file=sys.stderr)
        sys.exit(1)
    print(len(data))
except json.JSONDecodeError as e:
    print(f'Error: Invalid JSON in $CONFIG: {e}', file=sys.stderr)
    sys.exit(1)
")

# Get CGWindowID for Stellarr via CoreGraphics (no accessibility permission needed)
get_window_id() {
  swift -e '
    import CoreGraphics
    let windows = CGWindowListCopyWindowInfo(.optionOnScreenOnly, kCGNullWindowID) as? [[String: Any]] ?? []
    for w in windows {
      if let owner = w[kCGWindowOwnerName as String] as? String,
         owner == "Stellarr",
         let layer = w[kCGWindowLayer as String] as? Int,
         layer == 0,
         let wid = w[kCGWindowNumber as String] as? Int {
        print(wid)
        break
      }
    }
  ' 2>/dev/null || true
}

for i in $(seq 0 $((count - 1))); do
  name=$(python3 -c "import json; print(json.load(open('$CONFIG'))[$i]['name'])")
  output=$(python3 -c "import json; print(json.load(open('$CONFIG'))[$i]['output'])")

  echo "[$name] Launching app..."
  rm -f "$SIGNAL_FILE"

  # Write config entry for the app to read on startup
  python3 -c "import json; json.dump(json.load(open('$CONFIG'))[$i], open('$CONFIG_FILE', 'w'))"

  open "$APP"

  # Wait for ready signal
  elapsed=0
  while [ ! -f "$SIGNAL_FILE" ] && [ $elapsed -lt $TIMEOUT ]; do
    sleep 1
    elapsed=$((elapsed + 1))
  done

  if [ ! -f "$SIGNAL_FILE" ]; then
    echo "[$name] Timed out waiting for app to be ready."
    osascript -e 'tell application "Stellarr" to quit' 2>/dev/null || true
    sleep 2
    continue
  fi

  # Per-entry settle delay (ms). Lets meters / LUFS readings populate
  # when a test tone is playing. Default 1000ms matches old behaviour.
  settle_ms=$(python3 -c "import json; print(json.load(open('$CONFIG'))[$i].get('settleMs', 1000))")
  sleep "$(python3 -c "print($settle_ms / 1000)")"

  WINDOW_ID=$(get_window_id)

  if [ -n "$WINDOW_ID" ]; then
    screencapture -o -l"$WINDOW_ID" "$output"
    echo "[$name] Saved to $output"
  else
    echo "[$name] ERROR: Could not find Stellarr window."
  fi

  # Clean up
  rm -f "$SIGNAL_FILE" "$CONFIG_FILE"
  osascript -e 'tell application "Stellarr" to quit' 2>/dev/null || true
  sleep 2  # Wait for app to fully exit
done

echo "Done."

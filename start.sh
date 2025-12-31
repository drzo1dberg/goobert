#!/bin/bash
# Goobert + Stats Dashboard Launcher
# Starts both the video wall app and the web dashboard

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
DASHBOARD_DIR="$SCRIPT_DIR/dashboard"

echo "========================================"
echo "  Goobert + Stats Dashboard"
echo "========================================"
echo ""

# Check if build exists
if [ ! -f "$BUILD_DIR/goobert" ]; then
    echo "ERROR: Goobert not found at $BUILD_DIR/goobert"
    echo "Please build first: cd build && cmake .. && make"
    exit 1
fi

# Start dashboard in background
echo "Starting Stats Dashboard..."
cd "$DASHBOARD_DIR"
if [ ! -d "venv" ]; then
    echo "Creating Python virtual environment..."
    python3 -m venv venv
    source venv/bin/activate
    pip install -q flask flask-cors
else
    source venv/bin/activate
fi

# Kill any existing dashboard
pkill -f "python.*app.py" 2>/dev/null

# Start dashboard
python app.py > /tmp/goobert-dashboard.log 2>&1 &
DASHBOARD_PID=$!
echo "Dashboard started (PID: $DASHBOARD_PID)"

# Wait for dashboard to be ready
sleep 2

# Open Firefox to dashboard
echo "Opening Firefox..."
firefox http://localhost:5000 &

# Start Goobert
echo ""
echo "Starting Goobert..."
cd "$BUILD_DIR"
./goobert "$@"

# Cleanup: stop dashboard when Goobert exits
echo ""
echo "Goobert closed. Stopping dashboard..."
kill $DASHBOARD_PID 2>/dev/null

echo "Done."

#!/bin/bash
# Goobert Stats Dashboard - Start Script

cd "$(dirname "$0")"

# Check if venv exists, create if not
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
else
    source venv/bin/activate
fi

echo ""
echo "=========================================="
echo "  Goobert Stats Dashboard"
echo "=========================================="
echo ""
echo "Open in browser: http://localhost:5000"
echo ""

python app.py

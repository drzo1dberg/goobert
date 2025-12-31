#!/usr/bin/env python3
"""
Goobert Stats Dashboard - Web interface for viewing watch statistics

Connects to the SQLite database created by Goobert and displays
comprehensive statistics with modern visualizations.
"""

import sqlite3
import os
from datetime import datetime, timedelta
from pathlib import Path
from flask import Flask, render_template, jsonify, request
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# Database path
DB_PATH = os.path.expanduser("~/.config/goobert/goobert.db")


def get_db():
    """Get database connection"""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def format_duration(ms):
    """Format milliseconds to human readable string"""
    if ms is None:
        return "0s"
    seconds = ms // 1000
    if seconds < 60:
        return f"{seconds}s"
    elif seconds < 3600:
        return f"{seconds // 60}m {seconds % 60}s"
    else:
        hours = seconds // 3600
        mins = (seconds % 3600) // 60
        return f"{hours}h {mins}m"


@app.route('/')
def index():
    """Main dashboard page"""
    return render_template('index.html')


@app.route('/api/summary')
def api_summary():
    """Get summary statistics"""
    conn = get_db()
    cur = conn.cursor()

    # Total accumulated watch time (what we were showing before)
    cur.execute("SELECT COALESCE(SUM(total_watch_ms), 0) FROM file_stats")
    accumulated_watch_ms = cur.fetchone()[0]

    # Real elapsed time - based on actual session time spans
    # This accounts for parallel playback correctly
    cur.execute("""
        SELECT
            MIN(started_at) as first_start,
            MAX(ended_at) as last_end
        FROM watch_sessions
        WHERE started_at > 0
    """)
    row = cur.fetchone()
    if row and row['first_start'] and row['last_end']:
        real_elapsed_ms = row['last_end'] - row['first_start']
    else:
        real_elapsed_ms = 0

    # Today's real time (unique time ranges)
    today_start = int(datetime.now().replace(hour=0, minute=0, second=0).timestamp() * 1000)
    cur.execute("""
        SELECT
            MIN(started_at) as first_start,
            MAX(ended_at) as last_end
        FROM watch_sessions
        WHERE started_at >= ?
    """, (today_start,))
    row = cur.fetchone()
    today_real_ms = (row['last_end'] - row['first_start']) if row and row['first_start'] and row['last_end'] else 0

    # Session-based stats for today
    cur.execute("""
        SELECT COALESCE(SUM(duration_ms), 0), COUNT(*)
        FROM watch_sessions
        WHERE started_at >= ?
    """, (today_start,))
    row = cur.fetchone()
    today_accumulated_ms = row[0]
    today_sessions = row[1]

    # Files tracked
    cur.execute("SELECT COUNT(*) FROM file_stats WHERE total_watch_ms > 0")
    files_tracked = cur.fetchone()[0]

    # Total sessions
    cur.execute("SELECT COUNT(*) FROM watch_sessions")
    total_sessions = cur.fetchone()[0]

    # Average session length
    cur.execute("SELECT AVG(duration_ms) FROM watch_sessions WHERE duration_ms > 0")
    avg_session = cur.fetchone()[0] or 0

    # Peak hour
    cur.execute("""
        SELECT hour_of_day, SUM(duration_ms) as total
        FROM watch_sessions
        GROUP BY hour_of_day
        ORDER BY total DESC
        LIMIT 1
    """)
    row = cur.fetchone()
    peak_hour = row['hour_of_day'] if row else 0

    # Peak day
    cur.execute("""
        SELECT day_of_week, SUM(duration_ms) as total
        FROM watch_sessions
        GROUP BY day_of_week
        ORDER BY total DESC
        LIMIT 1
    """)
    row = cur.fetchone()
    peak_day = row['day_of_week'] if row else 1
    days = ['', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']
    peak_day_name = days[peak_day] if 1 <= peak_day <= 7 else 'N/A'

    # Total skips and screenshots
    cur.execute("SELECT COUNT(*) FROM skip_events")
    total_skips = cur.fetchone()[0]

    cur.execute("SELECT COUNT(*) FROM screenshot_events")
    total_screenshots = cur.fetchone()[0]

    # Parallelism factor (how much parallel playback on average)
    parallelism = accumulated_watch_ms / real_elapsed_ms if real_elapsed_ms > 0 else 1

    conn.close()

    return jsonify({
        'accumulated_watch_time': format_duration(accumulated_watch_ms),
        'accumulated_watch_ms': accumulated_watch_ms,
        'real_elapsed_time': format_duration(real_elapsed_ms),
        'real_elapsed_ms': real_elapsed_ms,
        'today_real_time': format_duration(today_real_ms),
        'today_accumulated_time': format_duration(today_accumulated_ms),
        'today_sessions': today_sessions,
        'parallelism_factor': round(parallelism, 2),
        'files_tracked': files_tracked,
        'total_sessions': total_sessions,
        'avg_session_length': format_duration(int(avg_session)),
        'peak_hour': f"{peak_hour:02d}:00 - {(peak_hour+1) % 24:02d}:00",
        'peak_day': peak_day_name,
        'total_skips': total_skips,
        'total_screenshots': total_screenshots
    })


@app.route('/api/hourly')
def api_hourly():
    """Get hourly distribution"""
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT hour_of_day, SUM(duration_ms) as total, COUNT(*) as sessions
        FROM watch_sessions
        GROUP BY hour_of_day
        ORDER BY hour_of_day
    """)

    # Initialize all 24 hours
    hourly = {h: {'total_ms': 0, 'sessions': 0} for h in range(24)}

    for row in cur.fetchall():
        hourly[row['hour_of_day']] = {
            'total_ms': row['total'],
            'sessions': row['sessions']
        }

    conn.close()

    return jsonify({
        'labels': [f"{h:02d}:00" for h in range(24)],
        'watch_time': [hourly[h]['total_ms'] / 1000 / 60 for h in range(24)],  # minutes
        'sessions': [hourly[h]['sessions'] for h in range(24)]
    })


@app.route('/api/daily')
def api_daily():
    """Get daily distribution (by day of week)"""
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT day_of_week, SUM(duration_ms) as total, COUNT(*) as sessions
        FROM watch_sessions
        GROUP BY day_of_week
        ORDER BY day_of_week
    """)

    days = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']
    daily = {d: {'total_ms': 0, 'sessions': 0} for d in range(1, 8)}

    for row in cur.fetchall():
        daily[row['day_of_week']] = {
            'total_ms': row['total'],
            'sessions': row['sessions']
        }

    conn.close()

    return jsonify({
        'labels': days,
        'watch_time': [daily[d]['total_ms'] / 1000 / 60 for d in range(1, 8)],  # minutes
        'sessions': [daily[d]['sessions'] for d in range(1, 8)]
    })


@app.route('/api/timeline')
def api_timeline():
    """Get timeline data - watch activity over time"""
    conn = get_db()
    cur = conn.cursor()

    # Last 30 days
    days = int(request.args.get('days', 30))

    cur.execute("""
        SELECT
            date(started_at / 1000, 'unixepoch', 'localtime') as day,
            SUM(duration_ms) as total_ms,
            COUNT(*) as sessions,
            MIN(started_at) as first_session,
            MAX(ended_at) as last_session
        FROM watch_sessions
        WHERE started_at >= ?
        GROUP BY day
        ORDER BY day
    """, (int((datetime.now() - timedelta(days=days)).timestamp() * 1000),))

    data = []
    for row in cur.fetchall():
        real_time = (row['last_session'] - row['first_session']) if row['first_session'] and row['last_session'] else 0
        data.append({
            'date': row['day'],
            'accumulated_minutes': row['total_ms'] / 1000 / 60,
            'real_minutes': real_time / 1000 / 60,
            'sessions': row['sessions']
        })

    conn.close()

    return jsonify(data)


@app.route('/api/top-files')
def api_top_files():
    """Get top watched files"""
    limit = int(request.args.get('limit', 20))

    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT
            file_path,
            total_watch_ms,
            play_count,
            last_watched_at,
            duration_ms,
            is_image
        FROM file_stats
        WHERE total_watch_ms > 0
        ORDER BY total_watch_ms DESC
        LIMIT ?
    """, (limit,))

    files = []
    for row in cur.fetchall():
        files.append({
            'file': os.path.basename(row['file_path']),
            'path': row['file_path'],
            'watch_time': format_duration(row['total_watch_ms']),
            'watch_ms': row['total_watch_ms'],
            'play_count': row['play_count'],
            'last_watched': datetime.fromtimestamp(row['last_watched_at'] / 1000).strftime('%Y-%m-%d %H:%M') if row['last_watched_at'] else 'Never',
            'duration': format_duration(row['duration_ms']),
            'is_image': bool(row['is_image'])
        })

    conn.close()
    return jsonify(files)


@app.route('/api/recent-sessions')
def api_recent_sessions():
    """Get recent watch sessions"""
    limit = int(request.args.get('limit', 50))

    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT
            fs.file_path,
            ws.started_at,
            ws.ended_at,
            ws.duration_ms,
            ws.cell_row,
            ws.cell_col,
            ws.hour_of_day
        FROM watch_sessions ws
        JOIN file_stats fs ON ws.file_id = fs.id
        ORDER BY ws.started_at DESC
        LIMIT ?
    """, (limit,))

    sessions = []
    for row in cur.fetchall():
        sessions.append({
            'file': os.path.basename(row['file_path']),
            'path': row['file_path'],
            'started': datetime.fromtimestamp(row['started_at'] / 1000).strftime('%Y-%m-%d %H:%M:%S') if row['started_at'] else 'N/A',
            'ended': datetime.fromtimestamp(row['ended_at'] / 1000).strftime('%H:%M:%S') if row['ended_at'] else 'N/A',
            'duration': format_duration(row['duration_ms']),
            'cell': f"[{row['cell_row']},{row['cell_col']}]",
            'hour': row['hour_of_day']
        })

    conn.close()
    return jsonify(sessions)


@app.route('/api/directories')
def api_directories():
    """Get directory statistics"""
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT
            file_path,
            total_watch_ms,
            play_count
        FROM file_stats
        WHERE total_watch_ms > 0
    """)

    dir_stats = {}
    for row in cur.fetchall():
        dir_path = os.path.dirname(row['file_path'])
        if dir_path not in dir_stats:
            dir_stats[dir_path] = {'watch_ms': 0, 'play_count': 0, 'file_count': 0}
        dir_stats[dir_path]['watch_ms'] += row['total_watch_ms']
        dir_stats[dir_path]['play_count'] += row['play_count']
        dir_stats[dir_path]['file_count'] += 1

    # Sort by watch time
    sorted_dirs = sorted(dir_stats.items(), key=lambda x: x[1]['watch_ms'], reverse=True)[:20]

    conn.close()

    return jsonify([{
        'directory': d[0],
        'short_name': os.path.basename(d[0]) or d[0],
        'watch_time': format_duration(d[1]['watch_ms']),
        'watch_ms': d[1]['watch_ms'],
        'play_count': d[1]['play_count'],
        'file_count': d[1]['file_count']
    } for d in sorted_dirs])


@app.route('/api/events')
def api_events():
    """Get recent events (skips, loops, pauses, etc.)"""
    limit = int(request.args.get('limit', 100))
    event_type = request.args.get('type', 'all')

    conn = get_db()
    cur = conn.cursor()

    events = []

    if event_type in ('all', 'skip'):
        cur.execute("""
            SELECT 'skip' as type, file_path, timestamp, skip_type as detail
            FROM skip_events
            ORDER BY timestamp DESC
            LIMIT ?
        """, (limit,))
        for row in cur.fetchall():
            events.append({
                'type': 'skip',
                'file': os.path.basename(row['file_path']),
                'time': datetime.fromtimestamp(row['timestamp'] / 1000).strftime('%Y-%m-%d %H:%M:%S'),
                'timestamp': row['timestamp'],
                'detail': row['detail']
            })

    if event_type in ('all', 'loop'):
        cur.execute("""
            SELECT 'loop' as type, file_path, timestamp, loop_enabled
            FROM loop_events
            ORDER BY timestamp DESC
            LIMIT ?
        """, (limit,))
        for row in cur.fetchall():
            events.append({
                'type': 'loop',
                'file': os.path.basename(row['file_path']),
                'time': datetime.fromtimestamp(row['timestamp'] / 1000).strftime('%Y-%m-%d %H:%M:%S'),
                'timestamp': row['timestamp'],
                'detail': 'enabled' if row['loop_enabled'] else 'disabled'
            })

    if event_type in ('all', 'fullscreen'):
        cur.execute("""
            SELECT timestamp, is_fullscreen, is_tile, cell_row, cell_col
            FROM fullscreen_events
            ORDER BY timestamp DESC
            LIMIT ?
        """, (limit,))
        for row in cur.fetchall():
            detail = 'enter' if row['is_fullscreen'] else 'exit'
            if row['is_tile']:
                detail += f" tile [{row['cell_row']},{row['cell_col']}]"
            events.append({
                'type': 'fullscreen',
                'file': '-',
                'time': datetime.fromtimestamp(row['timestamp'] / 1000).strftime('%Y-%m-%d %H:%M:%S'),
                'timestamp': row['timestamp'],
                'detail': detail
            })

    # Sort all events by timestamp
    events.sort(key=lambda x: x['timestamp'], reverse=True)

    conn.close()
    return jsonify(events[:limit])


@app.route('/api/grid-sessions')
def api_grid_sessions():
    """Get grid start/stop sessions"""
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT timestamp, rows, cols, source_path, filter, is_start
        FROM grid_events
        ORDER BY timestamp DESC
        LIMIT 50
    """)

    sessions = []
    for row in cur.fetchall():
        sessions.append({
            'time': datetime.fromtimestamp(row['timestamp'] / 1000).strftime('%Y-%m-%d %H:%M:%S'),
            'action': 'Started' if row['is_start'] else 'Stopped',
            'grid': f"{row['cols']}x{row['rows']}",
            'source': os.path.basename(row['source_path']) if row['source_path'] else '-',
            'filter': row['filter'] or '-'
        })

    conn.close()
    return jsonify(sessions)


if __name__ == '__main__':
    print(f"Goobert Stats Dashboard")
    print(f"Database: {DB_PATH}")
    print(f"Starting server at http://localhost:5000")
    app.run(debug=True, host='0.0.0.0', port=5000)

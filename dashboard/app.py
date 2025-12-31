#!/usr/bin/env python3
"""
Goobert Stats Dashboard - Web interface for viewing watch statistics

Connects to the SQLite database created by Goobert and displays
comprehensive statistics with modern visualizations.
"""

import sqlite3
import os
import subprocess
import platform
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
            fs.id,
            fs.file_path,
            fs.total_watch_ms,
            fs.play_count,
            fs.last_watched_at,
            fs.duration_ms,
            fs.is_image,
            fs.last_position_ms,
            COALESCE(skip_cnt.cnt, 0) as skip_count,
            COALESCE(loop_cnt.cnt, 0) as loop_count
        FROM file_stats fs
        LEFT JOIN (SELECT file_id, COUNT(*) as cnt FROM skip_events GROUP BY file_id) skip_cnt ON fs.id = skip_cnt.file_id
        LEFT JOIN (SELECT file_id, COUNT(*) as cnt FROM loop_events GROUP BY file_id) loop_cnt ON fs.id = loop_cnt.file_id
        WHERE fs.total_watch_ms > 0
        ORDER BY fs.total_watch_ms DESC
        LIMIT ?
    """, (limit,))

    files = []
    for row in cur.fetchall():
        duration_ms = row['duration_ms'] or 0
        last_pos_ms = row['last_position_ms'] or 0
        avg_pct = (last_pos_ms * 100.0 / duration_ms) if duration_ms > 0 else 0

        files.append({
            'file': os.path.basename(row['file_path']),
            'path': row['file_path'],
            'watch_time': format_duration(row['total_watch_ms']),
            'watch_ms': row['total_watch_ms'],
            'play_count': row['play_count'],
            'skip_count': row['skip_count'],
            'loop_count': row['loop_count'],
            'avg_watch_pct': round(avg_pct, 0),
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


@app.route('/api/completion-stats')
def api_completion_stats():
    """Get completion rate statistics"""
    conn = get_db()
    cur = conn.cursor()

    # Files watched >90%
    cur.execute("""
        SELECT COUNT(*) FROM file_stats
        WHERE duration_ms > 0
        AND (last_position_ms * 100.0 / duration_ms) >= 90
    """)
    full_watch = cur.fetchone()[0]

    # Files watched 10-90%
    cur.execute("""
        SELECT COUNT(*) FROM file_stats
        WHERE duration_ms > 0
        AND (last_position_ms * 100.0 / duration_ms) >= 10
        AND (last_position_ms * 100.0 / duration_ms) < 90
    """)
    partial_watch = cur.fetchone()[0]

    # Files watched <10%
    cur.execute("""
        SELECT COUNT(*) FROM file_stats
        WHERE duration_ms > 0
        AND (last_position_ms * 100.0 / duration_ms) < 10
    """)
    skipped = cur.fetchone()[0]

    # Average completion rate
    cur.execute("""
        SELECT AVG(CASE WHEN duration_ms > 0 THEN (last_position_ms * 100.0 / duration_ms) ELSE 0 END)
        FROM file_stats WHERE duration_ms > 0
    """)
    avg_completion = cur.fetchone()[0] or 0

    conn.close()

    return jsonify({
        'full_watch_count': full_watch,
        'partial_watch_count': partial_watch,
        'skipped_count': skipped,
        'average_completion': round(avg_completion, 1)
    })


@app.route('/api/skip-heatmap')
def api_skip_heatmap():
    """Get skip position heatmap - where users skip most often"""
    conn = get_db()
    cur = conn.cursor()

    # Get skip positions as percentage of file duration
    cur.execute("""
        SELECT
            CAST(se.from_position_ms * 100.0 / fs.duration_ms AS INTEGER) / 10 * 10 as bucket,
            COUNT(*) as cnt
        FROM skip_events se
        JOIN file_stats fs ON se.file_id = fs.id
        WHERE fs.duration_ms > 0
        GROUP BY bucket
        ORDER BY bucket
    """)

    # Initialize all buckets
    heatmap = {i: 0 for i in range(0, 110, 10)}
    for row in cur.fetchall():
        bucket = min(100, max(0, row['bucket']))
        heatmap[bucket] = row['cnt']

    conn.close()

    return jsonify({
        'labels': [f"{i}%" for i in range(0, 110, 10)],
        'values': [heatmap[i] for i in range(0, 110, 10)]
    })


@app.route('/api/position-heatmap')
def api_position_heatmap():
    """Get position viewing heatmap - which parts are watched most"""
    conn = get_db()
    cur = conn.cursor()

    # Check if position_samples table exists
    cur.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='position_samples'")
    if not cur.fetchone():
        conn.close()
        return jsonify({'labels': [], 'values': []})

    cur.execute("""
        SELECT position_pct, COUNT(*) as cnt
        FROM position_samples
        GROUP BY position_pct
        ORDER BY position_pct
    """)

    heatmap = {i: 0 for i in range(0, 105, 5)}
    for row in cur.fetchall():
        heatmap[row['position_pct']] = row['cnt']

    conn.close()

    return jsonify({
        'labels': [f"{i}%" for i in range(0, 105, 5)],
        'values': [heatmap[i] for i in range(0, 105, 5)]
    })


@app.route('/api/session-distribution')
def api_session_distribution():
    """Get session length distribution"""
    conn = get_db()
    cur = conn.cursor()

    cur.execute("SELECT duration_ms FROM watch_sessions WHERE duration_ms > 0")

    # Buckets: <30s, 30s-1m, 1-2m, 2-5m, 5-10m, 10-30m, 30m-1h, >1h
    buckets = {
        '<30s': 0,
        '30s-1m': 0,
        '1-2m': 0,
        '2-5m': 0,
        '5-10m': 0,
        '10-30m': 0,
        '30m-1h': 0,
        '>1h': 0
    }

    for row in cur.fetchall():
        duration_sec = row['duration_ms'] / 1000
        if duration_sec < 30:
            buckets['<30s'] += 1
        elif duration_sec < 60:
            buckets['30s-1m'] += 1
        elif duration_sec < 120:
            buckets['1-2m'] += 1
        elif duration_sec < 300:
            buckets['2-5m'] += 1
        elif duration_sec < 600:
            buckets['5-10m'] += 1
        elif duration_sec < 1800:
            buckets['10-30m'] += 1
        elif duration_sec < 3600:
            buckets['30m-1h'] += 1
        else:
            buckets['>1h'] += 1

    conn.close()

    return jsonify({
        'labels': list(buckets.keys()),
        'values': list(buckets.values())
    })


@app.route('/api/file-type-breakdown')
def api_file_type_breakdown():
    """Get video vs image time breakdown"""
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT is_image, SUM(total_watch_ms) as total, COUNT(*) as count
        FROM file_stats
        GROUP BY is_image
    """)

    video_ms = 0
    image_ms = 0
    video_count = 0
    image_count = 0

    for row in cur.fetchall():
        if row['is_image']:
            image_ms = row['total'] or 0
            image_count = row['count']
        else:
            video_ms = row['total'] or 0
            video_count = row['count']

    conn.close()

    return jsonify({
        'video_time': format_duration(video_ms),
        'video_ms': video_ms,
        'video_count': video_count,
        'image_time': format_duration(image_ms),
        'image_ms': image_ms,
        'image_count': image_count
    })


@app.route('/api/skip-types')
def api_skip_types():
    """Get skip type breakdown"""
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT skip_type, COUNT(*) as cnt
        FROM skip_events
        GROUP BY skip_type
        ORDER BY cnt DESC
    """)

    types = {}
    for row in cur.fetchall():
        types[row['skip_type'] or 'unknown'] = row['cnt']

    conn.close()

    return jsonify(types)


@app.route('/api/concurrent-stats')
def api_concurrent_stats():
    """Get concurrent cell statistics"""
    conn = get_db()
    cur = conn.cursor()

    # Average concurrent cells (estimated from parallelism factor)
    cur.execute("""
        SELECT
            COALESCE(SUM(duration_ms), 0) as total,
            (MAX(ended_at) - MIN(started_at)) as elapsed
        FROM watch_sessions
        WHERE started_at > 0
    """)
    row = cur.fetchone()
    avg_concurrent = (row['total'] / row['elapsed']) if row and row['elapsed'] else 1.0

    # Get cell usage distribution
    cur.execute("""
        SELECT cell_row, cell_col, COUNT(*) as sessions, SUM(duration_ms) as total_ms
        FROM watch_sessions
        GROUP BY cell_row, cell_col
        ORDER BY total_ms DESC
    """)

    cells = []
    for row in cur.fetchall():
        cells.append({
            'cell': f"[{row['cell_row']},{row['cell_col']}]",
            'sessions': row['sessions'],
            'watch_time': format_duration(row['total_ms'])
        })

    conn.close()

    return jsonify({
        'average_concurrent': round(avg_concurrent, 2),
        'cell_usage': cells[:16]  # Top 16 cells
    })


@app.route('/api/weekly-trend')
def api_weekly_trend():
    """Get weekly watch time trend"""
    weeks = int(request.args.get('weeks', 12))
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT
            strftime('%Y-W%W', started_at / 1000, 'unixepoch', 'localtime') as week,
            SUM(duration_ms) as total
        FROM watch_sessions
        WHERE started_at >= ?
        GROUP BY week
        ORDER BY week
    """, (int((datetime.now() - timedelta(weeks=weeks)).timestamp() * 1000),))

    data = []
    for row in cur.fetchall():
        data.append({
            'week': row['week'],
            'minutes': row['total'] / 1000 / 60
        })

    conn.close()
    return jsonify(data)


@app.route('/api/monthly-trend')
def api_monthly_trend():
    """Get monthly watch time trend"""
    months = int(request.args.get('months', 12))
    conn = get_db()
    cur = conn.cursor()

    # Calculate start date
    start_date = datetime.now() - timedelta(days=months * 30)

    cur.execute("""
        SELECT
            strftime('%Y-%m', started_at / 1000, 'unixepoch', 'localtime') as month,
            SUM(duration_ms) as total
        FROM watch_sessions
        WHERE started_at >= ?
        GROUP BY month
        ORDER BY month
    """, (int(start_date.timestamp() * 1000),))

    data = []
    for row in cur.fetchall():
        data.append({
            'month': row['month'],
            'minutes': row['total'] / 1000 / 60
        })

    conn.close()
    return jsonify(data)


@app.route('/api/favorites')
def api_favorites():
    """Get favorite files"""
    conn = get_db()
    cur = conn.cursor()

    # Check if favorites table exists
    cur.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='favorites'")
    if not cur.fetchone():
        conn.close()
        return jsonify([])

    cur.execute("""
        SELECT fs.file_path, fs.total_watch_ms, fs.play_count, fs.last_watched_at,
               fs.is_image, f.added_at
        FROM favorites f
        JOIN file_stats fs ON f.file_id = fs.id
        ORDER BY f.added_at DESC
    """)

    favorites = []
    for row in cur.fetchall():
        favorites.append({
            'file': os.path.basename(row['file_path']),
            'path': row['file_path'],
            'watch_time': format_duration(row['total_watch_ms']),
            'play_count': row['play_count'],
            'is_image': bool(row['is_image']),
            'last_watched': datetime.fromtimestamp(row['last_watched_at'] / 1000).strftime('%Y-%m-%d %H:%M') if row['last_watched_at'] else 'Never',
            'added_at': datetime.fromtimestamp(row['added_at'] / 1000).strftime('%Y-%m-%d %H:%M') if row['added_at'] else 'Unknown'
        })

    conn.close()
    return jsonify(favorites)


@app.route('/api/toggle-favorite', methods=['POST'])
def api_toggle_favorite():
    """Toggle favorite status for a file"""
    data = request.get_json()
    file_path = data.get('path')

    if not file_path:
        return jsonify({'error': 'No path provided'}), 400

    conn = get_db()
    cur = conn.cursor()

    # Check if favorites table exists, create if not
    cur.execute("""
        CREATE TABLE IF NOT EXISTS favorites (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER UNIQUE NOT NULL,
            added_at INTEGER DEFAULT (strftime('%s', 'now') * 1000),
            FOREIGN KEY (file_id) REFERENCES file_stats(id)
        )
    """)

    # Get file_id
    cur.execute("SELECT id FROM file_stats WHERE file_path = ?", (file_path,))
    row = cur.fetchone()
    if not row:
        conn.close()
        return jsonify({'error': 'File not found'}), 404

    file_id = row['id']

    # Check if already favorite
    cur.execute("SELECT id FROM favorites WHERE file_id = ?", (file_id,))
    if cur.fetchone():
        # Remove from favorites
        cur.execute("DELETE FROM favorites WHERE file_id = ?", (file_id,))
        is_favorite = False
    else:
        # Add to favorites
        cur.execute("INSERT INTO favorites (file_id) VALUES (?)", (file_id,))
        is_favorite = True

    conn.commit()
    conn.close()

    return jsonify({'is_favorite': is_favorite})


@app.route('/api/directory-analysis')
def api_directory_analysis():
    """Get enhanced directory statistics with session analysis"""
    conn = get_db()
    cur = conn.cursor()

    # Get all files grouped by directory
    cur.execute("""
        SELECT file_path, total_watch_ms, play_count
        FROM file_stats
        WHERE total_watch_ms > 0
    """)

    dir_stats = {}
    for row in cur.fetchall():
        dir_path = os.path.dirname(row['file_path'])
        if dir_path not in dir_stats:
            dir_stats[dir_path] = {
                'watch_ms': 0,
                'play_count': 0,
                'file_count': 0,
                'session_count': 0
            }
        dir_stats[dir_path]['watch_ms'] += row['total_watch_ms']
        dir_stats[dir_path]['play_count'] += row['play_count']
        dir_stats[dir_path]['file_count'] += 1

    # Get session counts per directory
    cur.execute("""
        SELECT fs.file_path, COUNT(*) as sessions
        FROM watch_sessions ws
        JOIN file_stats fs ON ws.file_id = fs.id
        GROUP BY fs.file_path
    """)

    for row in cur.fetchall():
        dir_path = os.path.dirname(row['file_path'])
        if dir_path in dir_stats:
            dir_stats[dir_path]['session_count'] += row['sessions']

    # Sort and format
    sorted_dirs = sorted(dir_stats.items(), key=lambda x: x[1]['watch_ms'], reverse=True)[:20]

    conn.close()

    return jsonify([{
        'directory': d[0],
        'short_name': os.path.basename(d[0]) or d[0],
        'watch_time': format_duration(d[1]['watch_ms']),
        'watch_ms': d[1]['watch_ms'],
        'play_count': d[1]['play_count'],
        'file_count': d[1]['file_count'],
        'session_count': d[1]['session_count'],
        'avg_session_per_file': round(d[1]['session_count'] / d[1]['file_count'], 1) if d[1]['file_count'] > 0 else 0
    } for d in sorted_dirs])


@app.route('/api/open-folder', methods=['POST'])
def api_open_folder():
    """Open a folder in the system file manager"""
    data = request.get_json()
    path = data.get('path')

    if not path:
        return jsonify({'error': 'No path provided'}), 400

    # Get directory from file path
    if os.path.isfile(path):
        folder = os.path.dirname(path)
    else:
        folder = path

    if not os.path.exists(folder):
        return jsonify({'error': 'Path does not exist'}), 404

    try:
        system = platform.system()
        if system == 'Linux':
            subprocess.Popen(['xdg-open', folder])
        elif system == 'Darwin':  # macOS
            subprocess.Popen(['open', folder])
        elif system == 'Windows':
            subprocess.Popen(['explorer', folder])
        else:
            return jsonify({'error': f'Unsupported platform: {system}'}), 500

        return jsonify({'success': True, 'folder': folder})
    except Exception as e:
        return jsonify({'error': str(e)}), 500


if __name__ == '__main__':
    print(f"Goobert Stats Dashboard")
    print(f"Database: {DB_PATH}")
    print(f"Starting server at http://localhost:5000")
    app.run(debug=True, host='0.0.0.0', port=5000)

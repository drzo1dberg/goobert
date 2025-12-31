#include "statsmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

StatsManager& StatsManager::instance()
{
    static StatsManager instance;
    return instance;
}

StatsManager::StatsManager()
    : QObject(nullptr)
{
}

StatsManager::~StatsManager()
{
    shutdown();
}

bool StatsManager::initialize()
{
    if (m_initialized) {
        return true;
    }

    // Get config directory
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString dbDir = configPath + "/goobert";
    QDir().mkpath(dbDir);
    QString dbPath = dbDir + "/goobert.db";

    // Open database
    m_db = QSqlDatabase::addDatabase("QSQLITE", "stats_connection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open stats database:" << m_db.lastError().text();
        return false;
    }

    // Enable WAL mode for better concurrency
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA journal_mode=WAL");

    if (!createTables()) {
        qWarning() << "Failed to create stats tables";
        return false;
    }

    // Setup periodic flush timer
    m_flushTimer = new QTimer(this);
    connect(m_flushTimer, &QTimer::timeout, this, &StatsManager::periodicFlush);
    m_flushTimer->start(StatsConstants::kFlushIntervalMs);

    m_initialized = true;
    qDebug() << "StatsManager initialized, database:" << dbPath;
    return true;
}

void StatsManager::shutdown()
{
    if (!m_initialized) {
        return;
    }

    if (m_flushTimer) {
        m_flushTimer->stop();
    }

    // Flush all active sessions
    stopAll();

    m_db.close();
    m_initialized = false;
}

bool StatsManager::createTables()
{
    QSqlQuery query(m_db);

    // File statistics table
    bool ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS file_stats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT UNIQUE NOT NULL,
            total_watch_ms INTEGER DEFAULT 0,
            play_count INTEGER DEFAULT 0,
            last_watched_at INTEGER,
            last_position_ms INTEGER DEFAULT 0,
            duration_ms INTEGER DEFAULT 0,
            is_image INTEGER DEFAULT 0,
            loop_toggle_count INTEGER DEFAULT 0,
            created_at INTEGER DEFAULT (strftime('%s', 'now') * 1000),
            updated_at INTEGER DEFAULT (strftime('%s', 'now') * 1000)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create file_stats table:" << query.lastError().text();
        return false;
    }

    // Watch sessions table - detailed session tracking
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS watch_sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            started_at INTEGER NOT NULL,
            ended_at INTEGER,
            duration_ms INTEGER DEFAULT 0,
            cell_row INTEGER,
            cell_col INTEGER,
            hour_of_day INTEGER,
            day_of_week INTEGER,
            FOREIGN KEY (file_id) REFERENCES file_stats(id)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create watch_sessions table:" << query.lastError().text();
        return false;
    }

    // Skip events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS skip_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            timestamp INTEGER NOT NULL,
            from_position_ms INTEGER,
            to_position_ms INTEGER,
            skip_type TEXT NOT NULL,
            FOREIGN KEY (file_id) REFERENCES file_stats(id)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create skip_events table:" << query.lastError().text();
        return false;
    }

    // Loop events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS loop_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            timestamp INTEGER NOT NULL,
            loop_enabled INTEGER NOT NULL,
            loop_count INTEGER DEFAULT 0,
            FOREIGN KEY (file_id) REFERENCES file_stats(id)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create loop_events table:" << query.lastError().text();
        return false;
    }

    // Rename history table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS rename_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            old_path TEXT NOT NULL,
            new_path TEXT NOT NULL,
            timestamp INTEGER NOT NULL
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create rename_history table:" << query.lastError().text();
        return false;
    }

    // Pause events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS pause_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER,
            timestamp INTEGER NOT NULL,
            position_ms INTEGER,
            pause_duration_ms INTEGER DEFAULT 0,
            is_pause INTEGER NOT NULL,
            FOREIGN KEY (file_id) REFERENCES file_stats(id)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create pause_events table:" << query.lastError().text();
        return false;
    }

    // Volume events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS volume_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            old_volume INTEGER,
            new_volume INTEGER,
            is_mute INTEGER DEFAULT 0
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create volume_events table:" << query.lastError().text();
        return false;
    }

    // Zoom events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS zoom_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER,
            timestamp INTEGER NOT NULL,
            zoom_level REAL DEFAULT 1.0,
            pan_x REAL DEFAULT 0.0,
            pan_y REAL DEFAULT 0.0,
            FOREIGN KEY (file_id) REFERENCES file_stats(id)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create zoom_events table:" << query.lastError().text();
        return false;
    }

    // Screenshot events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS screenshot_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER,
            timestamp INTEGER NOT NULL,
            position_ms INTEGER,
            screenshot_path TEXT,
            FOREIGN KEY (file_id) REFERENCES file_stats(id)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create screenshot_events table:" << query.lastError().text();
        return false;
    }

    // Fullscreen events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS fullscreen_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            is_fullscreen INTEGER NOT NULL,
            is_tile_fullscreen INTEGER DEFAULT 0,
            cell_row INTEGER DEFAULT -1,
            cell_col INTEGER DEFAULT -1
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create fullscreen_events table:" << query.lastError().text();
        return false;
    }

    // Grid events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS grid_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            rows INTEGER NOT NULL,
            cols INTEGER NOT NULL,
            source_path TEXT,
            filter TEXT,
            is_start INTEGER NOT NULL
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create grid_events table:" << query.lastError().text();
        return false;
    }

    // Rotation events table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS rotation_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER,
            timestamp INTEGER NOT NULL,
            rotation INTEGER DEFAULT 0,
            FOREIGN KEY (file_id) REFERENCES file_stats(id)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create rotation_events table:" << query.lastError().text();
        return false;
    }

    // Key bindings table
    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS key_bindings (
            action TEXT PRIMARY KEY,
            key_code INTEGER NOT NULL,
            modifiers INTEGER DEFAULT 0,
            updated_at INTEGER DEFAULT (strftime('%s', 'now') * 1000)
        )
    )");

    if (!ok) {
        qWarning() << "Failed to create key_bindings table:" << query.lastError().text();
        return false;
    }

    // Create indices
    query.exec("CREATE INDEX IF NOT EXISTS idx_file_stats_path ON file_stats(file_path)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_file_stats_last_watched ON file_stats(last_watched_at DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_file_stats_total_watch ON file_stats(total_watch_ms DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_watch_sessions_file ON watch_sessions(file_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_watch_sessions_started ON watch_sessions(started_at DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_watch_sessions_hour ON watch_sessions(hour_of_day)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_skip_events_file ON skip_events(file_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_skip_events_timestamp ON skip_events(timestamp DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_loop_events_file ON loop_events(file_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_rename_history_timestamp ON rename_history(timestamp DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_pause_events_file ON pause_events(file_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_pause_events_timestamp ON pause_events(timestamp DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_volume_events_timestamp ON volume_events(timestamp DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_zoom_events_file ON zoom_events(file_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_screenshot_events_timestamp ON screenshot_events(timestamp DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_fullscreen_events_timestamp ON fullscreen_events(timestamp DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_grid_events_timestamp ON grid_events(timestamp DESC)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_rotation_events_file ON rotation_events(file_id)");

    return true;
}

QString StatsManager::cellKey(int row, int col) const
{
    return QString("%1,%2").arg(row).arg(col);
}

qint64 StatsManager::getOrCreateFileId(const QString &filePath, double durationSec, bool isImage)
{
    QSqlQuery query(m_db);

    // Try to get existing
    query.prepare("SELECT id FROM file_stats WHERE file_path = ?");
    query.addBindValue(filePath);

    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }

    // Create new
    query.prepare(R"(
        INSERT INTO file_stats (file_path, duration_ms, is_image)
        VALUES (?, ?, ?)
    )");
    query.addBindValue(filePath);
    query.addBindValue(static_cast<qint64>(durationSec * 1000));
    query.addBindValue(isImage ? 1 : 0);

    if (query.exec()) {
        return query.lastInsertId().toLongLong();
    }

    qWarning() << "Failed to create file_stats entry:" << query.lastError().text();
    return -1;
}

void StatsManager::startWatching(int row, int col, const QString &filePath,
                                  double durationSec, bool isImage)
{
    if (!m_initialized || filePath.isEmpty()) {
        return;
    }

    QString key = cellKey(row, col);

    // Stop existing session for this cell
    if (m_activeSessions.contains(key)) {
        flushSession(key);
    }

    qint64 fileId = getOrCreateFileId(filePath, durationSec, isImage);
    if (fileId < 0) {
        return;
    }

    // Increment play count
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE file_stats
        SET play_count = play_count + 1,
            last_watched_at = ?,
            updated_at = ?
        WHERE id = ?
    )");
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    query.addBindValue(now);
    query.addBindValue(now);
    query.addBindValue(fileId);
    query.exec();

    // Create new session
    WatchSession session;
    session.fileId = fileId;
    session.filePath = filePath;
    session.startedAt = now;
    session.elapsed.start();
    session.cellRow = row;
    session.cellCol = col;
    session.isPaused = false;
    session.isImage = isImage;
    session.pausedDurationMs = 0;
    session.lastPositionSec = 0.0;
    session.durationSec = durationSec;

    m_activeSessions[key] = session;
}

void StatsManager::stopWatching(int row, int col)
{
    QString key = cellKey(row, col);
    if (m_activeSessions.contains(key)) {
        flushSession(key);
    }
}

void StatsManager::stopAll()
{
    QStringList keys = m_activeSessions.keys();
    for (const QString &key : keys) {
        flushSession(key);
    }
}

void StatsManager::updatePosition(int row, int col, double positionSec)
{
    QString key = cellKey(row, col);
    if (m_activeSessions.contains(key)) {
        m_activeSessions[key].lastPositionSec = positionSec;
    }
}

void StatsManager::setPaused(int row, int col, bool paused)
{
    QString key = cellKey(row, col);
    if (!m_activeSessions.contains(key)) {
        return;
    }

    WatchSession &session = m_activeSessions[key];

    // Images always count time even when "paused"
    if (session.isImage) {
        return;
    }

    if (paused && !session.isPaused) {
        // Starting pause
        session.pauseTimer.start();
        session.isPaused = true;
    } else if (!paused && session.isPaused) {
        // Ending pause
        session.pausedDurationMs += session.pauseTimer.elapsed();
        session.isPaused = false;
    }
}

void StatsManager::flushSession(const QString &cellKey)
{
    if (!m_activeSessions.contains(cellKey)) {
        return;
    }

    WatchSession session = m_activeSessions.take(cellKey);

    // Calculate watch duration
    qint64 totalElapsed = session.elapsed.elapsed();
    qint64 pausedTime = session.pausedDurationMs;

    // If currently paused, add current pause duration
    if (session.isPaused && !session.isImage) {
        pausedTime += session.pauseTimer.elapsed();
    }

    qint64 watchDuration = totalElapsed - pausedTime;

    // Ignore very short sessions
    if (watchDuration < StatsConstants::kMinSessionDurationMs) {
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QDateTime nowDt = QDateTime::fromMSecsSinceEpoch(now);

    // Update file_stats
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE file_stats
        SET total_watch_ms = total_watch_ms + ?,
            last_position_ms = ?,
            last_watched_at = ?,
            updated_at = ?
        WHERE id = ?
    )");

    query.addBindValue(watchDuration);
    query.addBindValue(static_cast<qint64>(session.lastPositionSec * 1000));
    query.addBindValue(now);
    query.addBindValue(now);
    query.addBindValue(session.fileId);

    if (!query.exec()) {
        qWarning() << "Failed to update file stats:" << query.lastError().text();
    }

    // Create watch_sessions entry
    QSqlQuery sessionQuery(m_db);
    sessionQuery.prepare(R"(
        INSERT INTO watch_sessions (file_id, started_at, ended_at, duration_ms, cell_row, cell_col, hour_of_day, day_of_week)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    sessionQuery.addBindValue(session.fileId);
    sessionQuery.addBindValue(session.startedAt);
    sessionQuery.addBindValue(now);
    sessionQuery.addBindValue(watchDuration);
    sessionQuery.addBindValue(session.cellRow);
    sessionQuery.addBindValue(session.cellCol);
    sessionQuery.addBindValue(nowDt.time().hour());
    sessionQuery.addBindValue(nowDt.date().dayOfWeek());

    if (!sessionQuery.exec()) {
        qWarning() << "Failed to insert watch session:" << sessionQuery.lastError().text();
    }

    emit statsUpdated(session.filePath);
}

void StatsManager::periodicFlush()
{
    // Periodically save progress for long-running sessions
    for (auto it = m_activeSessions.begin(); it != m_activeSessions.end(); ++it) {
        WatchSession &session = it.value();

        qint64 totalElapsed = session.elapsed.elapsed();
        qint64 pausedTime = session.pausedDurationMs;

        if (session.isPaused && !session.isImage) {
            pausedTime += session.pauseTimer.elapsed();
        }

        qint64 watchDuration = totalElapsed - pausedTime;

        if (watchDuration < StatsConstants::kMinSessionDurationMs) {
            continue;
        }

        // Update last position and watch time
        QSqlQuery query(m_db);
        query.prepare(R"(
            UPDATE file_stats
            SET total_watch_ms = total_watch_ms + ?,
                last_position_ms = ?,
                updated_at = ?
            WHERE id = ?
        )");

        qint64 now = QDateTime::currentMSecsSinceEpoch();
        query.addBindValue(watchDuration);
        query.addBindValue(static_cast<qint64>(session.lastPositionSec * 1000));
        query.addBindValue(now);
        query.addBindValue(session.fileId);
        query.exec();

        // Reset elapsed counters
        session.elapsed.restart();
        session.pausedDurationMs = 0;
        if (session.isPaused) {
            session.pauseTimer.restart();
        }
    }
}

FileStats StatsManager::getStats(const QString &filePath) const
{
    FileStats stats;
    if (!m_initialized) {
        return stats;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, file_path, total_watch_ms, play_count, last_watched_at,
               last_position_ms, duration_ms, is_image
        FROM file_stats
        WHERE file_path = ?
    )");
    query.addBindValue(filePath);

    if (query.exec() && query.next()) {
        stats.id = query.value(0).toLongLong();
        stats.filePath = query.value(1).toString();
        stats.totalWatchMs = query.value(2).toLongLong();
        stats.playCount = query.value(3).toInt();
        stats.lastWatchedAt = query.value(4).toLongLong();
        stats.lastPositionMs = query.value(5).toLongLong();
        stats.durationMs = query.value(6).toLongLong();
        stats.isImage = query.value(7).toBool();
    }

    return stats;
}

QList<FileStats> StatsManager::getMostWatched(int limit) const
{
    QList<FileStats> result;
    if (!m_initialized) {
        return result;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, file_path, total_watch_ms, play_count, last_watched_at,
               last_position_ms, duration_ms, is_image
        FROM file_stats
        WHERE total_watch_ms > 0
        ORDER BY total_watch_ms DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            FileStats stats;
            stats.id = query.value(0).toLongLong();
            stats.filePath = query.value(1).toString();
            stats.totalWatchMs = query.value(2).toLongLong();
            stats.playCount = query.value(3).toInt();
            stats.lastWatchedAt = query.value(4).toLongLong();
            stats.lastPositionMs = query.value(5).toLongLong();
            stats.durationMs = query.value(6).toLongLong();
            stats.isImage = query.value(7).toBool();
            result.append(stats);
        }
    }

    return result;
}

QList<FileStats> StatsManager::getRecentlyWatched(int limit) const
{
    QList<FileStats> result;
    if (!m_initialized) {
        return result;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, file_path, total_watch_ms, play_count, last_watched_at,
               last_position_ms, duration_ms, is_image
        FROM file_stats
        WHERE last_watched_at IS NOT NULL
        ORDER BY last_watched_at DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            FileStats stats;
            stats.id = query.value(0).toLongLong();
            stats.filePath = query.value(1).toString();
            stats.totalWatchMs = query.value(2).toLongLong();
            stats.playCount = query.value(3).toInt();
            stats.lastWatchedAt = query.value(4).toLongLong();
            stats.lastPositionMs = query.value(5).toLongLong();
            stats.durationMs = query.value(6).toLongLong();
            stats.isImage = query.value(7).toBool();
            result.append(stats);
        }
    }

    return result;
}

qint64 StatsManager::getTotalWatchTime() const
{
    if (!m_initialized) {
        return 0;
    }

    QSqlQuery query(m_db);
    if (query.exec("SELECT COALESCE(SUM(total_watch_ms), 0) FROM file_stats") && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

int StatsManager::getTotalFilesTracked() const
{
    if (!m_initialized) {
        return 0;
    }

    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM file_stats WHERE total_watch_ms > 0") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

double StatsManager::getLastPosition(const QString &filePath) const
{
    if (!m_initialized) {
        return 0.0;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT last_position_ms, duration_ms FROM file_stats WHERE file_path = ?");
    query.addBindValue(filePath);

    if (query.exec() && query.next()) {
        qint64 posMs = query.value(0).toLongLong();
        qint64 durMs = query.value(1).toLongLong();

        // Don't resume if near the end (>95%)
        if (durMs > 0 && posMs > durMs * 0.95) {
            return 0.0;
        }

        return posMs / 1000.0;
    }

    return 0.0;
}

bool StatsManager::exportToCsv(const QString &path) const
{
    if (!m_initialized) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "File Path,Total Watch Time (seconds),Play Count,Last Watched,Last Position (seconds),Duration (seconds),Is Image\n";

    QSqlQuery query(m_db);
    if (query.exec("SELECT file_path, total_watch_ms, play_count, last_watched_at, last_position_ms, duration_ms, is_image FROM file_stats ORDER BY total_watch_ms DESC")) {
        while (query.next()) {
            QString filePath = query.value(0).toString();
            filePath.replace("\"", "\"\"");  // Escape quotes

            qint64 lastWatched = query.value(3).toLongLong();
            QString lastWatchedStr = lastWatched > 0
                ? QDateTime::fromMSecsSinceEpoch(lastWatched).toString(Qt::ISODate)
                : "";

            out << "\"" << filePath << "\","
                << query.value(1).toLongLong() / 1000.0 << ","
                << query.value(2).toInt() << ","
                << "\"" << lastWatchedStr << "\","
                << query.value(4).toLongLong() / 1000.0 << ","
                << query.value(5).toLongLong() / 1000.0 << ","
                << (query.value(6).toBool() ? "Yes" : "No") << "\n";
        }
    }

    file.close();
    return true;
}

// ============ Event Logging Methods ============

void StatsManager::logSkipEvent(const QString &filePath, double fromPos, double toPos, const QString &skipType)
{
    if (!m_initialized || filePath.isEmpty()) return;

    qint64 fileId = getOrCreateFileId(filePath, 0, false);
    if (fileId < 0) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO skip_events (file_id, timestamp, from_position_ms, to_position_ms, skip_type)
        VALUES (?, ?, ?, ?, ?)
    )");
    query.addBindValue(fileId);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(static_cast<qint64>(fromPos * 1000));
    query.addBindValue(static_cast<qint64>(toPos * 1000));
    query.addBindValue(skipType);
    query.exec();
}

void StatsManager::logLoopToggle(const QString &filePath, bool loopEnabled, int loopCount)
{
    if (!m_initialized || filePath.isEmpty()) return;

    qint64 fileId = getOrCreateFileId(filePath, 0, false);
    if (fileId < 0) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO loop_events (file_id, timestamp, loop_enabled, loop_count)
        VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(fileId);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(loopEnabled ? 1 : 0);
    query.addBindValue(loopCount);
    query.exec();

    // Update loop toggle count in file_stats
    query.prepare("UPDATE file_stats SET loop_toggle_count = loop_toggle_count + 1 WHERE id = ?");
    query.addBindValue(fileId);
    query.exec();
}

void StatsManager::logRename(const QString &oldPath, const QString &newPath)
{
    if (!m_initialized) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO rename_history (old_path, new_path, timestamp)
        VALUES (?, ?, ?)
    )");
    query.addBindValue(oldPath);
    query.addBindValue(newPath);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.exec();

    // Update file_stats path if exists
    query.prepare("UPDATE file_stats SET file_path = ? WHERE file_path = ?");
    query.addBindValue(newPath);
    query.addBindValue(oldPath);
    query.exec();
}

void StatsManager::logPauseEvent(const QString &filePath, double positionSec, bool isPause)
{
    if (!m_initialized || filePath.isEmpty()) return;

    qint64 fileId = getOrCreateFileId(filePath, 0, false);
    if (fileId < 0) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO pause_events (file_id, timestamp, position_ms, is_pause)
        VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(fileId);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(static_cast<qint64>(positionSec * 1000));
    query.addBindValue(isPause ? 1 : 0);
    query.exec();
}

void StatsManager::logVolumeChange(int oldVolume, int newVolume, bool isMute)
{
    if (!m_initialized) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO volume_events (timestamp, old_volume, new_volume, is_mute)
        VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(oldVolume);
    query.addBindValue(newVolume);
    query.addBindValue(isMute ? 1 : 0);
    query.exec();
}

void StatsManager::logZoomEvent(const QString &filePath, double zoomLevel, double panX, double panY)
{
    if (!m_initialized || filePath.isEmpty()) return;

    qint64 fileId = getOrCreateFileId(filePath, 0, false);
    if (fileId < 0) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO zoom_events (file_id, timestamp, zoom_level, pan_x, pan_y)
        VALUES (?, ?, ?, ?, ?)
    )");
    query.addBindValue(fileId);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(zoomLevel);
    query.addBindValue(panX);
    query.addBindValue(panY);
    query.exec();
}

void StatsManager::logScreenshot(const QString &filePath, double positionSec, const QString &screenshotPath)
{
    if (!m_initialized || filePath.isEmpty()) return;

    qint64 fileId = getOrCreateFileId(filePath, 0, false);
    if (fileId < 0) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO screenshot_events (file_id, timestamp, position_ms, screenshot_path)
        VALUES (?, ?, ?, ?)
    )");
    query.addBindValue(fileId);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(static_cast<qint64>(positionSec * 1000));
    query.addBindValue(screenshotPath);
    query.exec();
}

void StatsManager::logFullscreenEvent(bool isFullscreen, bool isTile, int row, int col)
{
    if (!m_initialized) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO fullscreen_events (timestamp, is_fullscreen, is_tile_fullscreen, cell_row, cell_col)
        VALUES (?, ?, ?, ?, ?)
    )");
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(isFullscreen ? 1 : 0);
    query.addBindValue(isTile ? 1 : 0);
    query.addBindValue(row);
    query.addBindValue(col);
    query.exec();
}

void StatsManager::logGridEvent(int rows, int cols, const QString &sourcePath, const QString &filter, bool isStart)
{
    if (!m_initialized) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO grid_events (timestamp, rows, cols, source_path, filter, is_start)
        VALUES (?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(rows);
    query.addBindValue(cols);
    query.addBindValue(sourcePath);
    query.addBindValue(filter);
    query.addBindValue(isStart ? 1 : 0);
    query.exec();
}

void StatsManager::logRotation(const QString &filePath, int rotation)
{
    if (!m_initialized || filePath.isEmpty()) return;

    qint64 fileId = getOrCreateFileId(filePath, 0, false);
    if (fileId < 0) return;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO rotation_events (file_id, timestamp, rotation)
        VALUES (?, ?, ?)
    )");
    query.addBindValue(fileId);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(rotation);
    query.exec();
}

// ============ Session Query Methods ============

QList<WatchSessionInfo> StatsManager::getSessionsForFile(const QString &filePath, int limit) const
{
    QList<WatchSessionInfo> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT ws.id, ws.file_id, fs.file_path, ws.started_at, ws.ended_at,
               ws.duration_ms, ws.cell_row, ws.cell_col, ws.hour_of_day, ws.day_of_week
        FROM watch_sessions ws
        JOIN file_stats fs ON ws.file_id = fs.id
        WHERE fs.file_path = ?
        ORDER BY ws.started_at DESC
        LIMIT ?
    )");
    query.addBindValue(filePath);
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            WatchSessionInfo info;
            info.id = query.value(0).toLongLong();
            info.fileId = query.value(1).toLongLong();
            info.filePath = query.value(2).toString();
            info.startedAt = query.value(3).toLongLong();
            info.endedAt = query.value(4).toLongLong();
            info.durationMs = query.value(5).toLongLong();
            info.cellRow = query.value(6).toInt();
            info.cellCol = query.value(7).toInt();
            info.hourOfDay = query.value(8).toInt();
            info.dayOfWeek = query.value(9).toInt();
            result.append(info);
        }
    }
    return result;
}

QList<WatchSessionInfo> StatsManager::getRecentSessions(int limit) const
{
    QList<WatchSessionInfo> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT ws.id, ws.file_id, fs.file_path, ws.started_at, ws.ended_at,
               ws.duration_ms, ws.cell_row, ws.cell_col, ws.hour_of_day, ws.day_of_week
        FROM watch_sessions ws
        JOIN file_stats fs ON ws.file_id = fs.id
        ORDER BY ws.started_at DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            WatchSessionInfo info;
            info.id = query.value(0).toLongLong();
            info.fileId = query.value(1).toLongLong();
            info.filePath = query.value(2).toString();
            info.startedAt = query.value(3).toLongLong();
            info.endedAt = query.value(4).toLongLong();
            info.durationMs = query.value(5).toLongLong();
            info.cellRow = query.value(6).toInt();
            info.cellCol = query.value(7).toInt();
            info.hourOfDay = query.value(8).toInt();
            info.dayOfWeek = query.value(9).toInt();
            result.append(info);
        }
    }
    return result;
}

QList<HourlyStats> StatsManager::getHourlyDistribution() const
{
    QList<HourlyStats> result;
    if (!m_initialized) return result;

    // Initialize all 24 hours
    for (int h = 0; h < 24; ++h) {
        HourlyStats stats;
        stats.hour = h;
        result.append(stats);
    }

    QSqlQuery query(m_db);
    if (query.exec("SELECT hour_of_day, SUM(duration_ms), COUNT(*) FROM watch_sessions GROUP BY hour_of_day")) {
        while (query.next()) {
            int hour = query.value(0).toInt();
            if (hour >= 0 && hour < 24) {
                result[hour].totalWatchMs = query.value(1).toLongLong();
                result[hour].sessionCount = query.value(2).toInt();
            }
        }
    }
    return result;
}

QList<DailyStats> StatsManager::getDailyDistribution() const
{
    QList<DailyStats> result;
    if (!m_initialized) return result;

    // Initialize all 7 days (1=Mon, 7=Sun)
    for (int d = 1; d <= 7; ++d) {
        DailyStats stats;
        stats.dayOfWeek = d;
        result.append(stats);
    }

    QSqlQuery query(m_db);
    if (query.exec("SELECT day_of_week, SUM(duration_ms), COUNT(*) FROM watch_sessions GROUP BY day_of_week")) {
        while (query.next()) {
            int day = query.value(0).toInt();
            if (day >= 1 && day <= 7) {
                result[day - 1].totalWatchMs = query.value(1).toLongLong();
                result[day - 1].sessionCount = query.value(2).toInt();
            }
        }
    }
    return result;
}

qint64 StatsManager::getWatchTimeForDateRange(qint64 startMs, qint64 endMs) const
{
    if (!m_initialized) return 0;

    QSqlQuery query(m_db);
    query.prepare("SELECT COALESCE(SUM(duration_ms), 0) FROM watch_sessions WHERE started_at >= ? AND started_at <= ?");
    query.addBindValue(startMs);
    query.addBindValue(endMs);

    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

// ============ Event Query Methods ============

QList<SkipEvent> StatsManager::getSkipEvents(const QString &filePath, int limit) const
{
    QList<SkipEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    if (filePath.isEmpty()) {
        query.prepare(R"(
            SELECT se.id, se.file_id, fs.file_path, se.timestamp, se.from_position_ms, se.to_position_ms, se.skip_type
            FROM skip_events se
            JOIN file_stats fs ON se.file_id = fs.id
            ORDER BY se.timestamp DESC
            LIMIT ?
        )");
        query.addBindValue(limit);
    } else {
        query.prepare(R"(
            SELECT se.id, se.file_id, fs.file_path, se.timestamp, se.from_position_ms, se.to_position_ms, se.skip_type
            FROM skip_events se
            JOIN file_stats fs ON se.file_id = fs.id
            WHERE fs.file_path = ?
            ORDER BY se.timestamp DESC
            LIMIT ?
        )");
        query.addBindValue(filePath);
        query.addBindValue(limit);
    }

    if (query.exec()) {
        while (query.next()) {
            SkipEvent event;
            event.id = query.value(0).toLongLong();
            event.fileId = query.value(1).toLongLong();
            event.filePath = query.value(2).toString();
            event.timestamp = query.value(3).toLongLong();
            event.fromPositionSec = query.value(4).toLongLong() / 1000.0;
            event.toPositionSec = query.value(5).toLongLong() / 1000.0;
            event.skipType = query.value(6).toString();
            result.append(event);
        }
    }
    return result;
}

QList<LoopEvent> StatsManager::getLoopEvents(const QString &filePath, int limit) const
{
    QList<LoopEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    if (filePath.isEmpty()) {
        query.prepare(R"(
            SELECT le.id, le.file_id, fs.file_path, le.timestamp, le.loop_enabled, le.loop_count
            FROM loop_events le
            JOIN file_stats fs ON le.file_id = fs.id
            ORDER BY le.timestamp DESC
            LIMIT ?
        )");
        query.addBindValue(limit);
    } else {
        query.prepare(R"(
            SELECT le.id, le.file_id, fs.file_path, le.timestamp, le.loop_enabled, le.loop_count
            FROM loop_events le
            JOIN file_stats fs ON le.file_id = fs.id
            WHERE fs.file_path = ?
            ORDER BY le.timestamp DESC
            LIMIT ?
        )");
        query.addBindValue(filePath);
        query.addBindValue(limit);
    }

    if (query.exec()) {
        while (query.next()) {
            LoopEvent event;
            event.id = query.value(0).toLongLong();
            event.fileId = query.value(1).toLongLong();
            event.filePath = query.value(2).toString();
            event.timestamp = query.value(3).toLongLong();
            event.loopEnabled = query.value(4).toBool();
            event.loopCount = query.value(5).toInt();
            result.append(event);
        }
    }
    return result;
}

QList<RenameEvent> StatsManager::getRenameHistory(int limit) const
{
    QList<RenameEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, old_path, new_path, timestamp FROM rename_history ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            RenameEvent event;
            event.id = query.value(0).toLongLong();
            event.oldPath = query.value(1).toString();
            event.newPath = query.value(2).toString();
            event.timestamp = query.value(3).toLongLong();
            result.append(event);
        }
    }
    return result;
}

QList<PauseEvent> StatsManager::getPauseEvents(const QString &filePath, int limit) const
{
    QList<PauseEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    if (filePath.isEmpty()) {
        query.prepare(R"(
            SELECT pe.id, pe.file_id, fs.file_path, pe.timestamp, pe.position_ms, pe.pause_duration_ms, pe.is_pause
            FROM pause_events pe
            JOIN file_stats fs ON pe.file_id = fs.id
            ORDER BY pe.timestamp DESC
            LIMIT ?
        )");
        query.addBindValue(limit);
    } else {
        query.prepare(R"(
            SELECT pe.id, pe.file_id, fs.file_path, pe.timestamp, pe.position_ms, pe.pause_duration_ms, pe.is_pause
            FROM pause_events pe
            JOIN file_stats fs ON pe.file_id = fs.id
            WHERE fs.file_path = ?
            ORDER BY pe.timestamp DESC
            LIMIT ?
        )");
        query.addBindValue(filePath);
        query.addBindValue(limit);
    }

    if (query.exec()) {
        while (query.next()) {
            PauseEvent event;
            event.id = query.value(0).toLongLong();
            event.fileId = query.value(1).toLongLong();
            event.filePath = query.value(2).toString();
            event.timestamp = query.value(3).toLongLong();
            event.positionSec = query.value(4).toLongLong() / 1000.0;
            event.pauseDurationMs = query.value(5).toLongLong();
            event.isPause = query.value(6).toBool();
            result.append(event);
        }
    }
    return result;
}

QList<VolumeEvent> StatsManager::getVolumeHistory(int limit) const
{
    QList<VolumeEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, timestamp, old_volume, new_volume, is_mute FROM volume_events ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            VolumeEvent event;
            event.id = query.value(0).toLongLong();
            event.timestamp = query.value(1).toLongLong();
            event.oldVolume = query.value(2).toInt();
            event.newVolume = query.value(3).toInt();
            event.isMute = query.value(4).toBool();
            result.append(event);
        }
    }
    return result;
}

QList<ZoomEvent> StatsManager::getZoomEvents(const QString &filePath, int limit) const
{
    QList<ZoomEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    if (filePath.isEmpty()) {
        query.prepare(R"(
            SELECT ze.id, ze.file_id, ze.timestamp, ze.zoom_level, ze.pan_x, ze.pan_y
            FROM zoom_events ze
            ORDER BY ze.timestamp DESC
            LIMIT ?
        )");
        query.addBindValue(limit);
    } else {
        query.prepare(R"(
            SELECT ze.id, ze.file_id, ze.timestamp, ze.zoom_level, ze.pan_x, ze.pan_y
            FROM zoom_events ze
            JOIN file_stats fs ON ze.file_id = fs.id
            WHERE fs.file_path = ?
            ORDER BY ze.timestamp DESC
            LIMIT ?
        )");
        query.addBindValue(filePath);
        query.addBindValue(limit);
    }

    if (query.exec()) {
        while (query.next()) {
            ZoomEvent event;
            event.id = query.value(0).toLongLong();
            event.fileId = query.value(1).toLongLong();
            event.timestamp = query.value(2).toLongLong();
            event.zoomLevel = query.value(3).toDouble();
            event.panX = query.value(4).toDouble();
            event.panY = query.value(5).toDouble();
            result.append(event);
        }
    }
    return result;
}

QList<ScreenshotEvent> StatsManager::getScreenshotHistory(int limit) const
{
    QList<ScreenshotEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT se.id, se.file_id, fs.file_path, se.timestamp, se.position_ms, se.screenshot_path
        FROM screenshot_events se
        JOIN file_stats fs ON se.file_id = fs.id
        ORDER BY se.timestamp DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            ScreenshotEvent event;
            event.id = query.value(0).toLongLong();
            event.fileId = query.value(1).toLongLong();
            event.filePath = query.value(2).toString();
            event.timestamp = query.value(3).toLongLong();
            event.positionSec = query.value(4).toLongLong() / 1000.0;
            event.screenshotPath = query.value(5).toString();
            result.append(event);
        }
    }
    return result;
}

QList<FullscreenEvent> StatsManager::getFullscreenHistory(int limit) const
{
    QList<FullscreenEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, timestamp, is_fullscreen, is_tile_fullscreen, cell_row, cell_col FROM fullscreen_events ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            FullscreenEvent event;
            event.id = query.value(0).toLongLong();
            event.timestamp = query.value(1).toLongLong();
            event.isFullscreen = query.value(2).toBool();
            event.isTileFullscreen = query.value(3).toBool();
            event.cellRow = query.value(4).toInt();
            event.cellCol = query.value(5).toInt();
            result.append(event);
        }
    }
    return result;
}

QList<GridEvent> StatsManager::getGridHistory(int limit) const
{
    QList<GridEvent> result;
    if (!m_initialized) return result;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, timestamp, rows, cols, source_path, filter, is_start FROM grid_events ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            GridEvent event;
            event.id = query.value(0).toLongLong();
            event.timestamp = query.value(1).toLongLong();
            event.rows = query.value(2).toInt();
            event.cols = query.value(3).toInt();
            event.sourcePath = query.value(4).toString();
            event.filter = query.value(5).toString();
            event.isStart = query.value(6).toBool();
            result.append(event);
        }
    }
    return result;
}

int StatsManager::getLoopCountForFile(const QString &filePath) const
{
    if (!m_initialized) return 0;

    QSqlQuery query(m_db);
    query.prepare("SELECT loop_toggle_count FROM file_stats WHERE file_path = ?");
    query.addBindValue(filePath);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// ============ Analytics Methods ============

QList<CompletionStats> StatsManager::getCompletionStats(int limit) const
{
    QList<CompletionStats> result;
    if (!m_initialized) return result;

    // Calculate completion stats based on last_position_ms vs duration_ms
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT file_path,
               CASE WHEN duration_ms > 0 THEN (last_position_ms * 100.0 / duration_ms) ELSE 0 END as completion_pct,
               play_count,
               duration_ms
        FROM file_stats
        WHERE duration_ms > 0
        ORDER BY total_watch_ms DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            CompletionStats stats;
            stats.filePath = query.value(0).toString();
            stats.averageCompletionPercent = query.value(1).toDouble();
            int playCount = query.value(2).toInt();

            if (stats.averageCompletionPercent >= 90.0) {
                stats.fullWatchCount = playCount;
            } else if (stats.averageCompletionPercent < 10.0) {
                stats.skipCount = playCount;
            } else {
                stats.partialWatchCount = playCount;
            }
            result.append(stats);
        }
    }
    return result;
}

QList<DirectoryStats> StatsManager::getDirectoryStats(int limit) const
{
    QList<DirectoryStats> result;
    if (!m_initialized) return result;

    // Group stats by directory
    QSqlQuery query(m_db);
    if (query.exec("SELECT file_path, total_watch_ms, play_count FROM file_stats WHERE total_watch_ms > 0")) {
        QMap<QString, DirectoryStats> dirMap;

        while (query.next()) {
            QString path = query.value(0).toString();
            QString dir = path.section('/', 0, -2);  // Get directory part

            if (!dirMap.contains(dir)) {
                DirectoryStats stats;
                stats.directoryPath = dir;
                dirMap[dir] = stats;
            }

            dirMap[dir].totalWatchMs += query.value(1).toLongLong();
            dirMap[dir].fileCount++;
            dirMap[dir].playCount += query.value(2).toInt();
        }

        // Sort by watch time and take top limit
        QList<DirectoryStats> sorted = dirMap.values();
        std::sort(sorted.begin(), sorted.end(), [](const DirectoryStats &a, const DirectoryStats &b) {
            return a.totalWatchMs > b.totalWatchMs;
        });

        for (int i = 0; i < std::min(limit, static_cast<int>(sorted.size())); ++i) {
            result.append(sorted[i]);
        }
    }
    return result;
}

TimeRangeStats StatsManager::getStatsForToday() const
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime startOfDay = now.date().startOfDay();
    return getStatsForDateRange(startOfDay.toMSecsSinceEpoch(), now.toMSecsSinceEpoch());
}

TimeRangeStats StatsManager::getStatsForThisWeek() const
{
    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();
    QDate startOfWeek = today.addDays(-(today.dayOfWeek() - 1));  // Monday
    return getStatsForDateRange(startOfWeek.startOfDay().toMSecsSinceEpoch(), now.toMSecsSinceEpoch());
}

TimeRangeStats StatsManager::getStatsForThisMonth() const
{
    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();
    QDate startOfMonth = QDate(today.year(), today.month(), 1);
    return getStatsForDateRange(startOfMonth.startOfDay().toMSecsSinceEpoch(), now.toMSecsSinceEpoch());
}

TimeRangeStats StatsManager::getStatsForDateRange(qint64 startMs, qint64 endMs) const
{
    TimeRangeStats stats;
    stats.startTime = startMs;
    stats.endTime = endMs;

    if (!m_initialized) return stats;

    QSqlQuery query(m_db);

    // Watch time and sessions
    query.prepare("SELECT COALESCE(SUM(duration_ms), 0), COUNT(*), COUNT(DISTINCT file_id) FROM watch_sessions WHERE started_at >= ? AND started_at <= ?");
    query.addBindValue(startMs);
    query.addBindValue(endMs);
    if (query.exec() && query.next()) {
        stats.totalWatchMs = query.value(0).toLongLong();
        stats.sessionCount = query.value(1).toInt();
        stats.fileCount = query.value(2).toInt();
    }

    // Skip count
    query.prepare("SELECT COUNT(*) FROM skip_events WHERE timestamp >= ? AND timestamp <= ?");
    query.addBindValue(startMs);
    query.addBindValue(endMs);
    if (query.exec() && query.next()) {
        stats.skipCount = query.value(0).toInt();
    }

    // Loop count
    query.prepare("SELECT COUNT(*) FROM loop_events WHERE timestamp >= ? AND timestamp <= ? AND loop_enabled = 1");
    query.addBindValue(startMs);
    query.addBindValue(endMs);
    if (query.exec() && query.next()) {
        stats.loopCount = query.value(0).toInt();
    }

    return stats;
}

double StatsManager::getAverageSessionLength() const
{
    if (!m_initialized) return 0.0;

    QSqlQuery query(m_db);
    if (query.exec("SELECT AVG(duration_ms) FROM watch_sessions WHERE duration_ms > 0") && query.next()) {
        return query.value(0).toDouble();
    }
    return 0.0;
}

int StatsManager::getPeakHour() const
{
    if (!m_initialized) return 0;

    QSqlQuery query(m_db);
    if (query.exec("SELECT hour_of_day, SUM(duration_ms) as total FROM watch_sessions GROUP BY hour_of_day ORDER BY total DESC LIMIT 1") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int StatsManager::getPeakDayOfWeek() const
{
    if (!m_initialized) return 1;

    QSqlQuery query(m_db);
    if (query.exec("SELECT day_of_week, SUM(duration_ms) as total FROM watch_sessions GROUP BY day_of_week ORDER BY total DESC LIMIT 1") && query.next()) {
        return query.value(0).toInt();
    }
    return 1;
}

qint64 StatsManager::getLongestSession() const
{
    if (!m_initialized) return 0;

    QSqlQuery query(m_db);
    if (query.exec("SELECT MAX(duration_ms) FROM watch_sessions") && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

QString StatsManager::getMostWatchedDirectory() const
{
    if (!m_initialized) return QString();

    auto dirs = getDirectoryStats(1);
    if (!dirs.isEmpty()) {
        return dirs.first().directoryPath;
    }
    return QString();
}

double StatsManager::getAverageCompletionRate() const
{
    if (!m_initialized) return 0.0;

    QSqlQuery query(m_db);
    if (query.exec("SELECT AVG(CASE WHEN duration_ms > 0 THEN (last_position_ms * 100.0 / duration_ms) ELSE 0 END) FROM file_stats WHERE duration_ms > 0") && query.next()) {
        return query.value(0).toDouble();
    }
    return 0.0;
}

int StatsManager::getTotalScreenshots() const
{
    if (!m_initialized) return 0;

    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM screenshot_events") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int StatsManager::getTotalSkips() const
{
    if (!m_initialized) return 0;

    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM skip_events") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

qint64 StatsManager::getTotalPauseTime() const
{
    if (!m_initialized) return 0;

    QSqlQuery query(m_db);
    if (query.exec("SELECT COALESCE(SUM(pause_duration_ms), 0) FROM pause_events WHERE pause_duration_ms > 0") && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

// ============ Export Methods ============

bool StatsManager::exportSessionsToCsv(const QString &path) const
{
    if (!m_initialized) return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "Session ID,File Path,Started At,Ended At,Duration (seconds),Cell Row,Cell Col,Hour of Day,Day of Week\n";

    QSqlQuery query(m_db);
    if (query.exec(R"(
        SELECT ws.id, fs.file_path, ws.started_at, ws.ended_at, ws.duration_ms,
               ws.cell_row, ws.cell_col, ws.hour_of_day, ws.day_of_week
        FROM watch_sessions ws
        JOIN file_stats fs ON ws.file_id = fs.id
        ORDER BY ws.started_at DESC
    )")) {
        while (query.next()) {
            QString filePath = query.value(1).toString();
            filePath.replace("\"", "\"\"");

            qint64 startedAt = query.value(2).toLongLong();
            qint64 endedAt = query.value(3).toLongLong();

            out << query.value(0).toLongLong() << ","
                << "\"" << filePath << "\","
                << "\"" << QDateTime::fromMSecsSinceEpoch(startedAt).toString(Qt::ISODate) << "\","
                << "\"" << QDateTime::fromMSecsSinceEpoch(endedAt).toString(Qt::ISODate) << "\","
                << query.value(4).toLongLong() / 1000.0 << ","
                << query.value(5).toInt() << ","
                << query.value(6).toInt() << ","
                << query.value(7).toInt() << ","
                << query.value(8).toInt() << "\n";
        }
    }

    file.close();
    return true;
}

void StatsManager::clearAllStats()
{
    if (!m_initialized) return;

    stopAll();

    QSqlQuery query(m_db);
    query.exec("DELETE FROM watch_sessions");
    query.exec("DELETE FROM skip_events");
    query.exec("DELETE FROM loop_events");
    query.exec("DELETE FROM pause_events");
    query.exec("DELETE FROM volume_events");
    query.exec("DELETE FROM zoom_events");
    query.exec("DELETE FROM screenshot_events");
    query.exec("DELETE FROM fullscreen_events");
    query.exec("DELETE FROM grid_events");
    query.exec("DELETE FROM rotation_events");
    query.exec("DELETE FROM rename_history");
    query.exec("DELETE FROM file_stats");
}

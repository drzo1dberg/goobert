#pragma once

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QTimer>
#include <QMap>
#include <QElapsedTimer>
#include <QList>

struct FileStats {
    qint64 id = -1;
    QString filePath;
    qint64 totalWatchMs = 0;
    int playCount = 0;
    qint64 lastWatchedAt = 0;
    qint64 lastPositionMs = 0;
    qint64 durationMs = 0;
    bool isImage = false;
};

struct WatchSessionInfo {
    qint64 id = -1;
    qint64 fileId = -1;
    QString filePath;
    qint64 startedAt = 0;      // Unix timestamp ms
    qint64 endedAt = 0;        // Unix timestamp ms
    qint64 durationMs = 0;     // Actual watch time (excluding pause)
    int cellRow = 0;
    int cellCol = 0;
    int hourOfDay = 0;         // 0-23 for time analysis
    int dayOfWeek = 0;         // 1-7 (Mon-Sun)
};

struct HourlyStats {
    int hour = 0;              // 0-23
    qint64 totalWatchMs = 0;
    int sessionCount = 0;
};

struct DailyStats {
    int dayOfWeek = 0;         // 1-7 (Mon-Sun)
    qint64 totalWatchMs = 0;
    int sessionCount = 0;
};

struct SkipEvent {
    qint64 id = -1;
    qint64 fileId = -1;
    QString filePath;
    qint64 timestamp = 0;
    double fromPositionSec = 0.0;
    double toPositionSec = 0.0;
    QString skipType;          // "next", "prev", "seek_fwd", "seek_back", "shuffle"
};

struct LoopEvent {
    qint64 id = -1;
    qint64 fileId = -1;
    QString filePath;
    qint64 timestamp = 0;
    bool loopEnabled = false;
    int loopCount = 0;         // Total loops completed before toggle
};

struct RenameEvent {
    qint64 id = -1;
    QString oldPath;
    QString newPath;
    qint64 timestamp = 0;
};

struct PauseEvent {
    qint64 id = -1;
    qint64 fileId = -1;
    QString filePath;
    qint64 timestamp = 0;
    double positionSec = 0.0;
    qint64 pauseDurationMs = 0;
    bool isPause = true;       // true = pause, false = resume
};

struct VolumeEvent {
    qint64 id = -1;
    qint64 timestamp = 0;
    int oldVolume = 0;
    int newVolume = 0;
    bool isMute = false;
};

struct ZoomEvent {
    qint64 id = -1;
    qint64 fileId = -1;
    qint64 timestamp = 0;
    double zoomLevel = 1.0;
    double panX = 0.0;
    double panY = 0.0;
};

struct ScreenshotEvent {
    qint64 id = -1;
    qint64 fileId = -1;
    QString filePath;
    qint64 timestamp = 0;
    double positionSec = 0.0;
    QString screenshotPath;
};

struct FullscreenEvent {
    qint64 id = -1;
    qint64 timestamp = 0;
    bool isFullscreen = false;
    bool isTileFullscreen = false;
    int cellRow = -1;
    int cellCol = -1;
};

struct GridEvent {
    qint64 id = -1;
    qint64 timestamp = 0;
    int rows = 0;
    int cols = 0;
    QString sourcePath;
    QString filter;
    bool isStart = true;       // true = start, false = stop
};

struct CompletionStats {
    QString filePath;
    double averageCompletionPercent = 0.0;
    int fullWatchCount = 0;    // Watched > 90%
    int partialWatchCount = 0; // Watched < 90%
    int skipCount = 0;         // Skipped before 10%
};

struct DirectoryStats {
    QString directoryPath;
    qint64 totalWatchMs = 0;
    int fileCount = 0;
    int playCount = 0;
};

struct TimeRangeStats {
    qint64 startTime = 0;
    qint64 endTime = 0;
    qint64 totalWatchMs = 0;
    int sessionCount = 0;
    int fileCount = 0;
    int skipCount = 0;
    int loopCount = 0;
};

namespace StatsConstants {
    inline constexpr int kFlushIntervalMs = 10000;
    inline constexpr int kMinSessionDurationMs = 1000;
}

class StatsManager : public QObject
{
    Q_OBJECT

public:
    [[nodiscard]] static StatsManager& instance();

    // Database lifecycle
    bool initialize();
    void shutdown();
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    // Watch tracking
    void startWatching(int row, int col, const QString &filePath,
                       double durationSec, bool isImage);
    void stopWatching(int row, int col);
    void updatePosition(int row, int col, double positionSec);
    void setPaused(int row, int col, bool paused);
    void stopAll();

    // Statistics queries
    [[nodiscard]] FileStats getStats(const QString &filePath) const;
    [[nodiscard]] QList<FileStats> getMostWatched(int limit = 20) const;
    [[nodiscard]] QList<FileStats> getRecentlyWatched(int limit = 20) const;
    [[nodiscard]] qint64 getTotalWatchTime() const;
    [[nodiscard]] int getTotalFilesTracked() const;

    // Resume support
    [[nodiscard]] double getLastPosition(const QString &filePath) const;

    // Session queries
    [[nodiscard]] QList<WatchSessionInfo> getSessionsForFile(const QString &filePath, int limit = 100) const;
    [[nodiscard]] QList<WatchSessionInfo> getRecentSessions(int limit = 100) const;
    [[nodiscard]] QList<HourlyStats> getHourlyDistribution() const;
    [[nodiscard]] QList<DailyStats> getDailyDistribution() const;
    [[nodiscard]] qint64 getWatchTimeForDateRange(qint64 startMs, qint64 endMs) const;

    // Event logging - actions
    void logSkipEvent(const QString &filePath, double fromPos, double toPos, const QString &skipType);
    void logLoopToggle(const QString &filePath, bool loopEnabled, int loopCount = 0);
    void logRename(const QString &oldPath, const QString &newPath);
    void logPauseEvent(const QString &filePath, double positionSec, bool isPause);
    void logVolumeChange(int oldVolume, int newVolume, bool isMute = false);
    void logZoomEvent(const QString &filePath, double zoomLevel, double panX, double panY);
    void logScreenshot(const QString &filePath, double positionSec, const QString &screenshotPath);
    void logFullscreenEvent(bool isFullscreen, bool isTile, int row = -1, int col = -1);
    void logGridEvent(int rows, int cols, const QString &sourcePath, const QString &filter, bool isStart);
    void logRotation(const QString &filePath, int rotation);

    // Event queries
    [[nodiscard]] QList<SkipEvent> getSkipEvents(const QString &filePath = QString(), int limit = 100) const;
    [[nodiscard]] QList<LoopEvent> getLoopEvents(const QString &filePath = QString(), int limit = 100) const;
    [[nodiscard]] QList<RenameEvent> getRenameHistory(int limit = 100) const;
    [[nodiscard]] QList<PauseEvent> getPauseEvents(const QString &filePath = QString(), int limit = 100) const;
    [[nodiscard]] QList<VolumeEvent> getVolumeHistory(int limit = 100) const;
    [[nodiscard]] QList<ZoomEvent> getZoomEvents(const QString &filePath = QString(), int limit = 100) const;
    [[nodiscard]] QList<ScreenshotEvent> getScreenshotHistory(int limit = 100) const;
    [[nodiscard]] QList<FullscreenEvent> getFullscreenHistory(int limit = 100) const;
    [[nodiscard]] QList<GridEvent> getGridHistory(int limit = 100) const;
    [[nodiscard]] int getLoopCountForFile(const QString &filePath) const;

    // Analytics queries
    [[nodiscard]] QList<CompletionStats> getCompletionStats(int limit = 50) const;
    [[nodiscard]] QList<DirectoryStats> getDirectoryStats(int limit = 20) const;
    [[nodiscard]] TimeRangeStats getStatsForToday() const;
    [[nodiscard]] TimeRangeStats getStatsForThisWeek() const;
    [[nodiscard]] TimeRangeStats getStatsForThisMonth() const;
    [[nodiscard]] TimeRangeStats getStatsForDateRange(qint64 startMs, qint64 endMs) const;
    [[nodiscard]] double getAverageSessionLength() const;
    [[nodiscard]] int getPeakHour() const;
    [[nodiscard]] int getPeakDayOfWeek() const;
    [[nodiscard]] qint64 getLongestSession() const;
    [[nodiscard]] QString getMostWatchedDirectory() const;
    [[nodiscard]] double getAverageCompletionRate() const;
    [[nodiscard]] int getTotalScreenshots() const;
    [[nodiscard]] int getTotalSkips() const;
    [[nodiscard]] qint64 getTotalPauseTime() const;

    // Export/Clear
    bool exportToCsv(const QString &path) const;
    bool exportSessionsToCsv(const QString &path) const;
    void clearAllStats();

signals:
    void statsUpdated(const QString &filePath);

private:
    StatsManager();
    ~StatsManager() override;
    StatsManager(const StatsManager&) = delete;
    StatsManager& operator=(const StatsManager&) = delete;

    struct WatchSession {
        qint64 fileId = -1;
        QString filePath;
        qint64 startedAt = 0;
        QElapsedTimer elapsed;
        int cellRow = 0;
        int cellCol = 0;
        bool isPaused = false;
        bool isImage = false;
        qint64 pausedDurationMs = 0;
        QElapsedTimer pauseTimer;
        double lastPositionSec = 0.0;
        double durationSec = 0.0;
    };

    bool createTables();
    qint64 getOrCreateFileId(const QString &filePath, double durationSec, bool isImage);
    void flushSession(const QString &cellKey);
    void periodicFlush();
    [[nodiscard]] QString cellKey(int row, int col) const;

    QSqlDatabase m_db;
    QMap<QString, WatchSession> m_activeSessions;
    QTimer *m_flushTimer = nullptr;
    bool m_initialized = false;
};

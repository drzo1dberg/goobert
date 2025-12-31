#pragma once

#include <QString>
#include <QSettings>

class Config
{
public:
    [[nodiscard]] static Config& instance();

    // Playback settings
    [[nodiscard]] int loopCount() const noexcept { return m_loopCount; }
    void setLoopCount(int count) { m_loopCount = count; save(); }

    [[nodiscard]] int defaultVolume() const noexcept { return m_defaultVolume; }
    void setDefaultVolume(int vol) { m_defaultVolume = vol; save(); }

    [[nodiscard]] double imageDisplayDuration() const noexcept { return m_imageDisplayDuration; }
    void setImageDisplayDuration(double dur) { m_imageDisplayDuration = dur; save(); }

    [[nodiscard]] int volumeStep() const noexcept { return m_volumeStep; }
    void setVolumeStep(int step) { m_volumeStep = step; save(); }

    // Seek settings
    [[nodiscard]] double seekStepSeconds() const noexcept { return m_seekStepSeconds; }
    void setSeekStepSeconds(double secs) { m_seekStepSeconds = secs; save(); }

    [[nodiscard]] double seekStepLongSeconds() const noexcept { return m_seekStepLongSeconds; }
    void setSeekStepLongSeconds(double secs) { m_seekStepLongSeconds = secs; save(); }

    [[nodiscard]] int seekAmountSeconds() const noexcept { return m_seekAmountSeconds; }
    void setSeekAmountSeconds(int seconds) { m_seekAmountSeconds = seconds; save(); }

    // Video control settings
    [[nodiscard]] double zoomStep() const noexcept { return m_zoomStep; }
    void setZoomStep(double step) { m_zoomStep = step; save(); }

    [[nodiscard]] int rotationStep() const noexcept { return m_rotationStep; }
    void setRotationStep(int step) { m_rotationStep = step; save(); }

    [[nodiscard]] int osdDurationMs() const noexcept { return m_osdDurationMs; }
    void setOsdDurationMs(int ms) { m_osdDurationMs = ms; save(); }

    [[nodiscard]] int watchdogIntervalMs() const noexcept { return m_watchdogIntervalMs; }
    void setWatchdogIntervalMs(int ms) { m_watchdogIntervalMs = ms; save(); }

    // Grid settings
    [[nodiscard]] int defaultRows() const noexcept { return m_defaultRows; }
    void setDefaultRows(int rows) { m_defaultRows = rows; save(); }

    [[nodiscard]] int defaultCols() const noexcept { return m_defaultCols; }
    void setDefaultCols(int cols) { m_defaultCols = cols; save(); }

    [[nodiscard]] int maxGridSize() const noexcept { return m_maxGridSize; }
    void setMaxGridSize(int size) { m_maxGridSize = size; save(); }

    [[nodiscard]] int gridSpacing() const noexcept { return m_gridSpacing; }
    void setGridSpacing(int spacing) { m_gridSpacing = spacing; save(); }

    // Path settings
    [[nodiscard]] QString defaultMediaPath() const { return m_defaultMediaPath; }
    void setDefaultMediaPath(const QString &path) { m_defaultMediaPath = path; save(); }

    [[nodiscard]] QString screenshotPath() const { return m_screenshotPath; }
    void setScreenshotPath(const QString &path) { m_screenshotPath = path; save(); }

    // Skipper settings
    [[nodiscard]] bool skipperEnabled() const noexcept { return m_skipperEnabled; }
    void setSkipperEnabled(bool enabled) { m_skipperEnabled = enabled; save(); }

    [[nodiscard]] double skipPercent() const noexcept { return m_skipPercent; }
    void setSkipPercent(double percent) { m_skipPercent = percent; save(); }

    // Statistics settings
    [[nodiscard]] bool statsEnabled() const noexcept { return m_statsEnabled; }
    void setStatsEnabled(bool enabled) { m_statsEnabled = enabled; save(); }

    [[nodiscard]] bool resumePlaybackEnabled() const noexcept { return m_resumePlaybackEnabled; }
    void setResumePlaybackEnabled(bool enabled) { m_resumePlaybackEnabled = enabled; save(); }

    void load();
    void save();
    void resetToDefaults();

private:
    Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // Playback
    int m_loopCount = 5;
    int m_defaultVolume = 30;
    double m_imageDisplayDuration = 2.5;
    int m_volumeStep = 5;

    // Seek
    double m_seekStepSeconds = 5.0;
    double m_seekStepLongSeconds = 120.0;
    int m_seekAmountSeconds = 30;

    // Video controls
    double m_zoomStep = 0.1;
    int m_rotationStep = 90;
    int m_osdDurationMs = 1500;
    int m_watchdogIntervalMs = 5000;

    // Grid
    int m_defaultRows = 3;
    int m_defaultCols = 3;
    int m_maxGridSize = 10;
    int m_gridSpacing = 2;

    // Path
    QString m_defaultMediaPath;
    QString m_screenshotPath;

    // Skipper
    bool m_skipperEnabled = true;
    double m_skipPercent = 0.33;

    // Statistics
    bool m_statsEnabled = true;
    bool m_resumePlaybackEnabled = true;
};

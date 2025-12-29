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

    // Grid settings
    [[nodiscard]] int defaultRows() const noexcept { return m_defaultRows; }
    void setDefaultRows(int rows) { m_defaultRows = rows; save(); }

    [[nodiscard]] int defaultCols() const noexcept { return m_defaultCols; }
    void setDefaultCols(int cols) { m_defaultCols = cols; save(); }

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

    // Seek amount (for horizontal scroll)
    [[nodiscard]] int seekAmountSeconds() const noexcept { return m_seekAmountSeconds; }
    void setSeekAmountSeconds(int seconds) { m_seekAmountSeconds = seconds; save(); }

    void load();
    void save();

private:
    Config();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // Playback
    int m_loopCount = 5;
    int m_defaultVolume = 30;
    double m_imageDisplayDuration = 2.5;

    // Grid
    int m_defaultRows = 3;
    int m_defaultCols = 3;

    // Path
    QString m_defaultMediaPath;
    QString m_screenshotPath;

    // Skipper
    bool m_skipperEnabled = true;
    double m_skipPercent = 0.33;

    // Seek
    int m_seekAmountSeconds = 30;
};

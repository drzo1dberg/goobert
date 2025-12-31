#include "config.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

Config& Config::instance()
{
    static Config instance;
    return instance;
}

Config::Config()
{
    load();
}

void Config::load()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString configPath = configDir + "/goobert/goobert.ini";

    QSettings settings(configPath, QSettings::IniFormat);

    // Playback
    m_loopCount = settings.value("playback/loop_count", 5).toInt();
    m_defaultVolume = settings.value("playback/default_volume", 30).toInt();
    m_imageDisplayDuration = settings.value("playback/image_display_duration", 2.5).toDouble();
    m_volumeStep = settings.value("playback/volume_step", 5).toInt();

    // Seek
    m_seekStepSeconds = settings.value("seek/step_seconds", 5.0).toDouble();
    m_seekStepLongSeconds = settings.value("seek/step_long_seconds", 120.0).toDouble();
    m_seekAmountSeconds = settings.value("seek/amount_seconds", 30).toInt();

    // Video controls
    m_zoomStep = settings.value("video/zoom_step", 0.1).toDouble();
    m_rotationStep = settings.value("video/rotation_step", 90).toInt();
    m_osdDurationMs = settings.value("video/osd_duration_ms", 1500).toInt();
    m_watchdogIntervalMs = settings.value("video/watchdog_interval_ms", 5000).toInt();

    // Grid
    m_defaultRows = settings.value("grid/default_rows", 3).toInt();
    m_defaultCols = settings.value("grid/default_cols", 3).toInt();
    m_maxGridSize = settings.value("grid/max_size", 10).toInt();
    m_gridSpacing = settings.value("grid/spacing", 2).toInt();

    // Path
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    m_defaultMediaPath = settings.value("paths/default_media_path", defaultPath).toString();

    QString defaultScreenshotPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    m_screenshotPath = settings.value("paths/screenshot_path", defaultScreenshotPath).toString();

    // Skipper
    m_skipperEnabled = settings.value("skipper/enabled", true).toBool();
    m_skipPercent = settings.value("skipper/skip_percent", 0.33).toDouble();

    // Statistics
    m_statsEnabled = settings.value("stats/enabled", true).toBool();
    m_resumePlaybackEnabled = settings.value("stats/resume_playback", true).toBool();

    qDebug() << "Config loaded from:" << configPath;
}

void Config::save()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir().mkpath(configDir + "/goobert");
    QString configPath = configDir + "/goobert/goobert.ini";

    QSettings settings(configPath, QSettings::IniFormat);

    // Playback
    settings.setValue("playback/loop_count", m_loopCount);
    settings.setValue("playback/default_volume", m_defaultVolume);
    settings.setValue("playback/image_display_duration", m_imageDisplayDuration);
    settings.setValue("playback/volume_step", m_volumeStep);

    // Seek
    settings.setValue("seek/step_seconds", m_seekStepSeconds);
    settings.setValue("seek/step_long_seconds", m_seekStepLongSeconds);
    settings.setValue("seek/amount_seconds", m_seekAmountSeconds);

    // Video controls
    settings.setValue("video/zoom_step", m_zoomStep);
    settings.setValue("video/rotation_step", m_rotationStep);
    settings.setValue("video/osd_duration_ms", m_osdDurationMs);
    settings.setValue("video/watchdog_interval_ms", m_watchdogIntervalMs);

    // Grid
    settings.setValue("grid/default_rows", m_defaultRows);
    settings.setValue("grid/default_cols", m_defaultCols);
    settings.setValue("grid/max_size", m_maxGridSize);
    settings.setValue("grid/spacing", m_gridSpacing);

    // Path
    settings.setValue("paths/default_media_path", m_defaultMediaPath);
    settings.setValue("paths/screenshot_path", m_screenshotPath);

    // Skipper
    settings.setValue("skipper/enabled", m_skipperEnabled);
    settings.setValue("skipper/skip_percent", m_skipPercent);

    // Statistics
    settings.setValue("stats/enabled", m_statsEnabled);
    settings.setValue("stats/resume_playback", m_resumePlaybackEnabled);

    settings.sync();
    qDebug() << "Config saved to:" << configPath;
}

void Config::resetToDefaults()
{
    // Playback
    m_loopCount = 5;
    m_defaultVolume = 30;
    m_imageDisplayDuration = 2.5;
    m_volumeStep = 5;

    // Seek
    m_seekStepSeconds = 5.0;
    m_seekStepLongSeconds = 120.0;
    m_seekAmountSeconds = 30;

    // Video controls
    m_zoomStep = 0.1;
    m_rotationStep = 90;
    m_osdDurationMs = 1500;
    m_watchdogIntervalMs = 5000;

    // Grid
    m_defaultRows = 3;
    m_defaultCols = 3;
    m_maxGridSize = 10;
    m_gridSpacing = 2;

    // Path - keep user's paths
    // m_defaultMediaPath = ...
    // m_screenshotPath = ...

    // Skipper
    m_skipperEnabled = true;
    m_skipPercent = 0.33;

    // Statistics
    m_statsEnabled = true;
    m_resumePlaybackEnabled = true;

    save();
}

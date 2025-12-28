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

    // Grid
    m_defaultRows = settings.value("grid/default_rows", 3).toInt();
    m_defaultCols = settings.value("grid/default_cols", 3).toInt();

    // Path
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    m_defaultMediaPath = settings.value("paths/default_media_path", defaultPath).toString();

    // Skipper
    m_skipperEnabled = settings.value("skipper/enabled", true).toBool();
    m_skipPercent = settings.value("skipper/skip_percent", 0.33).toDouble();

    // Seek
    m_seekAmountSeconds = settings.value("seek/amount_seconds", 30).toInt();

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

    // Grid
    settings.setValue("grid/default_rows", m_defaultRows);
    settings.setValue("grid/default_cols", m_defaultCols);

    // Path
    settings.setValue("paths/default_media_path", m_defaultMediaPath);

    // Skipper
    settings.setValue("skipper/enabled", m_skipperEnabled);
    settings.setValue("skipper/skip_percent", m_skipPercent);

    // Seek
    settings.setValue("seek/amount_seconds", m_seekAmountSeconds);

    settings.sync();
    qDebug() << "Config saved to:" << configPath;
}

#pragma once

#include <QStringList>
#include <QSet>

class FileScanner
{
public:
    FileScanner() = default;

    [[nodiscard]] QStringList scan(const QString &path) const;
    [[nodiscard]] QStringList scan(const QString &path, const QString &filter) const;

    // Filter: space-separated AND terms, case-insensitive
    [[nodiscard]] static QStringList applyFilter(const QStringList &files, const QString &filter);

    [[nodiscard]] static const QSet<QString>& videoExtensions() noexcept;
    [[nodiscard]] static const QSet<QString>& imageExtensions() noexcept;

private:
    static inline const QSet<QString> s_videoExts = {
        "mkv", "mp4", "avi", "mov", "m4v", "flv", "wmv", "mpg", "mpeg", "ts", "ogv", "webm"
    };
    static inline const QSet<QString> s_imageExts = {
        "jpg", "jpeg", "png", "webp", "avif", "bmp", "tif", "tiff", "gif"
    };
};

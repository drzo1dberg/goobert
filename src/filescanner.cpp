#include "filescanner.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

QSet<QString> FileScanner::s_videoExts = {
    "mkv", "mp4", "avi", "mov", "m4v", "flv", "wmv", "mpg", "mpeg", "ts", "ogv", "webm"
};

QSet<QString> FileScanner::s_imageExts = {
    "jpg", "jpeg", "png", "webp", "avif", "bmp", "tif", "tiff", "gif"
};

FileScanner::FileScanner()
{
}

QStringList FileScanner::scan(const QString &path) const
{
    QStringList result;
    QFileInfo fi(path);

    if (!fi.exists()) {
        return result;
    }

    if (fi.isFile()) {
        QString ext = fi.suffix().toLower();
        if (s_videoExts.contains(ext) || s_imageExts.contains(ext)) {
            result.append(fi.absoluteFilePath());
        }
        return result;
    }

    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString ext = it.fileInfo().suffix().toLower();
        if (s_videoExts.contains(ext) || s_imageExts.contains(ext)) {
            result.append(it.filePath());
        }
    }

    result.sort();
    return result;
}

QStringList FileScanner::scan(const QString &path, const QString &filter) const
{
    QStringList files = scan(path);
    if (filter.isEmpty()) {
        return files;
    }
    return applyFilter(files, filter);
}

QStringList FileScanner::applyFilter(const QStringList &files, const QString &filter)
{
    if (filter.isEmpty()) {
        return files;
    }

    // Split filter into AND terms (space-separated)
    QStringList terms = filter.toLower().split(' ', Qt::SkipEmptyParts);
    if (terms.isEmpty()) {
        return files;
    }

    QStringList result;
    for (const QString &file : files) {
        QString filename = QFileInfo(file).fileName().toLower();

        // All terms must match (AND)
        bool allMatch = true;
        for (const QString &term : terms) {
            if (!filename.contains(term)) {
                allMatch = false;
                break;
            }
        }

        if (allMatch) {
            result.append(file);
        }
    }

    return result;
}

const QSet<QString> &FileScanner::videoExtensions()
{
    return s_videoExts;
}

const QSet<QString> &FileScanner::imageExtensions()
{
    return s_imageExts;
}

#include "filescanner.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <algorithm>
#include <ranges>

QStringList FileScanner::scan(const QString &path) const
{
    QStringList result;
    QFileInfo fi(path);

    if (!fi.exists()) {
        return result;
    }

    if (fi.isFile()) {
        if (QString ext = fi.suffix().toLower(); s_videoExts.contains(ext) || s_imageExts.contains(ext)) {
            result.append(fi.absoluteFilePath());
        }
        return result;
    }

    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (QString ext = it.fileInfo().suffix().toLower(); s_videoExts.contains(ext) || s_imageExts.contains(ext)) {
            result.append(it.filePath());
        }
    }

    result.sort();
    return result;
}

QStringList FileScanner::scan(const QString &path, const QString &filter) const
{
    QStringList files = scan(path);
    return filter.isEmpty() ? files : applyFilter(files, filter);
}

QStringList FileScanner::applyFilter(const QStringList &files, const QString &filter)
{
    if (filter.isEmpty()) {
        return files;
    }

    // Split filter into AND terms (space-separated)
    const QStringList terms = filter.toLower().split(' ', Qt::SkipEmptyParts);
    if (terms.isEmpty()) {
        return files;
    }

    QStringList result;
    for (const QString &file : files) {
        const QString filename = QFileInfo(file).fileName().toLower();

        // All terms must match (AND) - using std::ranges::all_of
        bool allMatch = std::ranges::all_of(terms, [&filename](const QString &term) {
            return filename.contains(term);
        });

        if (allMatch) {
            result.append(file);
        }
    }

    return result;
}

const QSet<QString>& FileScanner::videoExtensions() noexcept
{
    return s_videoExts;
}

const QSet<QString>& FileScanner::imageExtensions() noexcept
{
    return s_imageExts;
}

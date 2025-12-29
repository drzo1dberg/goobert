#pragma once

#include <QStringList>
#include <QSet>

class FileScanner
{
public:
    FileScanner();

    QStringList scan(const QString &path) const;
    QStringList scan(const QString &path, const QString &filter) const;

    // Filter: space-separated AND terms, case-insensitive
    static QStringList applyFilter(const QStringList &files, const QString &filter);

    static const QSet<QString> &videoExtensions();
    static const QSet<QString> &imageExtensions();

private:
    static QSet<QString> s_videoExts;
    static QSet<QString> s_imageExts;
};

#pragma once

#include <QStringList>
#include <QSet>

class FileScanner
{
public:
    FileScanner();

    QStringList scan(const QString &path) const;

    static const QSet<QString> &videoExtensions();
    static const QSet<QString> &imageExtensions();

private:
    static QSet<QString> s_videoExts;
    static QSet<QString> s_imageExts;
};

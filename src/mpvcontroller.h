#pragma once

#include <QObject>
#include <QString>
#include <QLocalSocket>
#include <QJsonObject>

class MpvController : public QObject
{
    Q_OBJECT

public:
    explicit MpvController(const QString &socketPath, QObject *parent = nullptr);
    ~MpvController();

    bool connect();
    void disconnect();
    bool isConnected() const;

    QJsonObject sendCommand(const QStringList &args);
    QVariant getProperty(const QString &name);
    bool setProperty(const QString &name, const QVariant &value);

    void playlistNext();
    void playlistPrev();
    void playlistShuffle();
    void togglePause();
    void setVolume(int volume);
    void toggleMute();

signals:
    void connected();
    void disconnected();
    void error(const QString &message);

private:
    QString m_socketPath;
    QLocalSocket *m_socket = nullptr;
};

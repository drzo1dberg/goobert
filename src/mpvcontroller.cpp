#include "mpvcontroller.h"
#include <QJsonDocument>
#include <QJsonArray>

MpvController::MpvController(const QString &socketPath, QObject *parent)
    : QObject(parent)
    , m_socketPath(socketPath)
{
    m_socket = new QLocalSocket(this);

    QObject::connect(m_socket, &QLocalSocket::connected, this, &MpvController::connected);
    QObject::connect(m_socket, &QLocalSocket::disconnected, this, &MpvController::disconnected);
    QObject::connect(m_socket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError err) {
        Q_UNUSED(err);
        emit error(m_socket->errorString());
    });
}

MpvController::~MpvController()
{
    disconnect();
}

bool MpvController::connect()
{
    m_socket->connectToServer(m_socketPath);
    return m_socket->waitForConnected(1000);
}

void MpvController::disconnect()
{
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnectFromServer();
    }
}

bool MpvController::isConnected() const
{
    return m_socket->state() == QLocalSocket::ConnectedState;
}

QJsonObject MpvController::sendCommand(const QStringList &args)
{
    if (!isConnected() && !connect()) {
        return QJsonObject{{"error", "not connected"}};
    }

    QJsonArray cmdArray;
    for (const QString &arg : args) {
        cmdArray.append(arg);
    }

    QJsonObject cmd;
    cmd["command"] = cmdArray;

    QByteArray data = QJsonDocument(cmd).toJson(QJsonDocument::Compact) + "\n";
    m_socket->write(data);
    m_socket->flush();

    if (!m_socket->waitForReadyRead(1000)) {
        return QJsonObject{{"error", "timeout"}};
    }

    QByteArray response = m_socket->readLine();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    return doc.object();
}

QVariant MpvController::getProperty(const QString &name)
{
    QJsonObject result = sendCommand({"get_property", name});
    if (result.contains("data")) {
        return result["data"].toVariant();
    }
    return QVariant();
}

bool MpvController::setProperty(const QString &name, const QVariant &value)
{
    QStringList args = {"set_property", name};

    switch (value.typeId()) {
    case QMetaType::Bool:
        args.append(value.toBool() ? "yes" : "no");
        break;
    default:
        args.append(value.toString());
        break;
    }

    QJsonObject result = sendCommand(args);
    return result.value("error").toString() == "success";
}

void MpvController::playlistNext()
{
    sendCommand({"playlist-next", "force"});
}

void MpvController::playlistPrev()
{
    sendCommand({"playlist-prev"});
}

void MpvController::playlistShuffle()
{
    sendCommand({"playlist-shuffle"});
}

void MpvController::togglePause()
{
    sendCommand({"cycle", "pause"});
}

void MpvController::setVolume(int volume)
{
    setProperty("volume", volume);
}

void MpvController::toggleMute()
{
    sendCommand({"cycle", "mute"});
}

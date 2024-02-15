#include "messageserver.hpp"


namespace Erc
{

namespace Private
{


MessageServer::~MessageServer()
{
    if (m_server)
        m_server->close();
}

MessageServer::MessageServer(const QString& key, QLocalServer::SocketOption permissions, QObject* parent)
    : QObject(parent)
{
    QLocalServer::removeServer(key);
    m_server = new QLocalServer(this);

    // restrict access to the socket
    m_server->setSocketOptions(permissions);

    m_server->listen(key);

    QObject::connect(m_server, &QLocalServer::newConnection, this, &MessageServer::connectionEstablished);
}

void MessageServer::connectionEstablished()
{
    QLocalSocket* nextConnSocket = m_server->nextPendingConnection();
    m_connectionMap.insert(nextConnSocket, ConnectionInfo());

    QObject::connect(
        nextConnSocket,
        &QLocalSocket::aboutToClose,
        this,
        [nextConnSocket, this]()
        {
            auto& info = m_connectionMap[nextConnSocket];
            this->connectionClosed(nextConnSocket);
        }
    );

    QObject::connect(
        nextConnSocket,
        &QLocalSocket::disconnected,
        nextConnSocket,
        &QLocalSocket::deleteLater
    );

    QObject::connect(
        nextConnSocket,
        &QLocalSocket::destroyed,
        this,
        [nextConnSocket, this]()
        {
            m_connectionMap.remove(nextConnSocket);
        }
    );

    QObject::connect(
        nextConnSocket,
        &QLocalSocket::readyRead,
        this,
        [nextConnSocket, this]()
        {
            auto& info = m_connectionMap[nextConnSocket];
            this->dataAvailable(nextConnSocket);
        }
    );
}

void MessageServer::connectionClosed(QLocalSocket* s)
{
    if (s->bytesAvailable() > 0)
        dataAvailable(s);
}

void MessageServer::dataAvailable(QLocalSocket* s)
{
    const QByteArray message = s->readAll();

    emit receivedMessage(message);
}


} // namespace Private {}

} // namespace Erc {}
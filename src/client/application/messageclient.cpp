#include <QElapsedTimer>
#include <QThread>

#include "messageclient.hpp"


namespace Erp
{

namespace Client
{

MessageClient::~MessageClient()
{
    if (m_socket)
        m_socket->close();
}

MessageClient::MessageClient(const QString& key, QObject* parent)
    : QObject(parent)
    , m_key(key)
{

}

bool MessageClient::connect(int msecTimeout)
{
    QElapsedTimer time;
    time.start();

    if (!m_socket)
    {
        m_socket = new QLocalSocket(this);
    }

    if (m_socket->state() == QLocalSocket::ConnectedState)
    {
        return true;
    }

    if (m_socket->state() != QLocalSocket::ConnectedState)
    {
        while (true)
        {
            QThread::msleep(10);

            if (m_socket->state() != QLocalSocket::ConnectingState)
            {
                m_socket->connectToServer(m_key);
            }

            if (m_socket->state() == QLocalSocket::ConnectingState)
            {
                m_socket->waitForConnected(static_cast<int>(msecTimeout - time.elapsed()));
            }

            if (m_socket->state() == QLocalSocket::ConnectedState)
                break;

            if (time.elapsed() >= msecTimeout)
                return false;
        }
    }

    return true;
}

bool MessageClient::writeMessage(int msecTimeout, const QByteArray& msg)
{
    if (!connect(msecTimeout))
        return false;

    auto size = static_cast<qint32>(msg.size());
    while (size > 0)
    {
        auto w = m_socket->write(msg);
        if (w <= 0)
            return false;

        size -= w;
    }

    return m_socket->flush();
}


} // namespace Client {}

} // namespace Erp {}

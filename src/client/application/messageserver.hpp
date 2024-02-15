#pragma once


#include <QLocalServer>
#include <QLocalSocket>
#include <QString>


namespace Erc
{

namespace Private
{


class MessageServer final
    : public QObject
{
    Q_OBJECT

public:
    ~MessageServer();

    explicit MessageServer(const QString& key, QLocalServer::SocketOption permissions, QObject* parent);

signals:
    void receivedMessage(QByteArray message);

private slots:
    void connectionEstablished();
    void connectionClosed(QLocalSocket* s);
    void dataAvailable(QLocalSocket* s);

private:
    struct ConnectionInfo
    {

    };

    QLocalServer* m_server = nullptr;
    QMap<QLocalSocket*, ConnectionInfo> m_connectionMap;
};


} // namespace Private {}

} // namespace Erc {}

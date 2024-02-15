#pragma once

#include <QLocalSocket>
#include <QString>


namespace Erc
{

namespace Private
{


class MessageClient final
    : public QObject
{
    Q_OBJECT

public:
    ~MessageClient();

    explicit MessageClient(const QString& key, QObject* parent);

    bool connect(int msecTimeout);
    bool writeMessage(int msecTimeout, const QByteArray& msg);

private:
    QString m_key;
    QLocalSocket* m_socket = nullptr;
};


} // namespace Private {}

} // namespace Erc {}
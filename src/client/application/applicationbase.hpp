#pragma once

#include <QApplication>

#include "messageclient.hpp"
#include "messageserver.hpp"
#include "sharedsection.hpp"

namespace Erp
{

namespace Client
{


class ApplicationBase
    : public QApplication
{
    Q_OBJECT

public:
    ~ApplicationBase();
    explicit ApplicationBase(int& argc, char** argv);

    bool primary() const {return !!m_server; }
    bool secondary() const { return !m_server; }
    uint64_t primaryPid();

    bool sendMessage(const QByteArray& message, int timeout);

signals:
    void receivedMessage(QByteArray message);

private:
    struct alignas(8) InstanceInfo
    {
        bool primary = false;
        uint32_t secondary = 0;
        uint64_t primaryPid = 0;
        uint16_t checksum = 0; // Must be the last field

        void init()
        {
            primary = false;
            secondary = 0;
            primaryPid = -1;
            checksum = calcChecksum();
        }

        uint16_t calcChecksum() const
        {
            return ::qChecksum(QByteArray(reinterpret_cast<const char*>(this), offsetof(InstanceInfo, checksum)));
        }
    };

    static QString generateKey();

    void startPrimary(InstanceInfo* instanceInfo);
    void startSecondary(InstanceInfo* instanceInfo);

    static const QString kAppKey;

    QString m_key;
    SharedSection<InstanceInfo> m_sharedSection;
    MessageServer* m_server = nullptr;
    MessageClient* m_client = nullptr;
};



} // namespace Client {}

} // namespace Erp {}

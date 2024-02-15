#include <erebus/system/user.hxx>

#include <QByteArray>
#include <QElapsedTimer>
#include <QThread>

#include "applicationbase.hpp"


namespace Erc
{

namespace Private
{

const QString ApplicationBase::kAppKey = QStringLiteral("48B54B65FC794FFF8159C31C1943F252");


ApplicationBase::~ApplicationBase()
{
    if (primary())
    {
        auto l = m_sharedSection.lock();
        l.get()->init();
    }
}

ApplicationBase::ApplicationBase(int& argc, char** argv)
    : QApplication(argc, argv)
    , m_key(generateKey())
    , m_sharedSection(m_key)
{
    if (m_sharedSection.owner()) // we're the primary instance
    {
        auto l = m_sharedSection.lock();
        l.get()->init();
    }

    // make sure the shared memory block is initialized and in consistent state
    QElapsedTimer time;
    time.start();

    while (true)
    {
        {
            auto l = m_sharedSection.lock();

            // if the shared memory block's checksum is valid continue
            if(l.get()->calcChecksum() == l.get()->checksum)
                break;

            // if more than 5s have elapsed, assume the primary instance crashed and
            // assume it's position
            if (time.elapsed() > 5000)
            {
                qWarning() << "Shared memory block has been in an inconsistent state from more than 5s. Assuming primary instance failure.";
                l.get()->init();
            }

            // otherwise wait and try again
        }

        QThread::msleep(50);
    }

    auto l = m_sharedSection.lock();
    if (!l.get()->primary)
    {
        startPrimary(l.get());

        return;
    }

    startSecondary(l.get());
}

QString ApplicationBase::generateKey()
{
    QString key(kAppKey);

    key.append(QString::fromUtf8(Er::System::CurrentUser::name()));

    QByteArray ba;
    ba.append(key.toUtf8());

    return ba.toBase64().replace("/", "_");
}

void ApplicationBase::startPrimary(InstanceInfo* instanceInfo)
{
    instanceInfo->primary = true;
    instanceInfo->primaryPid = QCoreApplication::applicationPid();
    instanceInfo->checksum = instanceInfo->calcChecksum();

    m_server = new MessageServer(m_key, QLocalServer::UserAccessOption, this);

    QObject::connect(m_server, &MessageServer::receivedMessage, this, &ApplicationBase::receivedMessage);
}

void ApplicationBase::startSecondary(InstanceInfo* instanceInfo)
{
    instanceInfo->secondary += 1;
    instanceInfo->checksum = instanceInfo->calcChecksum();
}

bool ApplicationBase::sendMessage(const QByteArray& message, int timeout)
{
     // nobody to connect to
    if (primary())
        return false;

    if (!m_client)
        m_client = new MessageClient(m_key, this);

    return m_client->writeMessage(timeout, message);
}

uint64_t ApplicationBase::primaryPid()
{
    uint64_t pid = 0;

    auto l = m_sharedSection.lock();
    pid = l.get()->primaryPid;

    return pid;
}



} // namespace Private {}

} // namespace Erc {}
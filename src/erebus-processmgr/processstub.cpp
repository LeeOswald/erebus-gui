#include "processstub.hpp"

#include <erebus/util/exceptionutil.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>


namespace Erp::ProcessMgr
{

namespace
{

class ProcessStubImpl
    : public IProcessStub
    , public Er::NonCopyable
{
public:
    ~ProcessStubImpl()
    {
    }

    explicit ProcessStubImpl(Er::Client::ChannelPtr channel, Er::Log::ILog* log)
        : m_client(Er::Client::createClient(channel, log))
        , m_log(log)
    {
    }

    PosixResult kill(uint64_t pid, std::string_view signame) override
    {
        return Er::protectedCall<PosixResult>(
            m_log,
            [this, pid, signame]()
            {
                Er::PropertyBag request;
                Er::addProperty<Er::ProcessMgr::Props::Pid>(request, pid);
                Er::addProperty<Er::ProcessMgr::Props::SignalName>(request, std::string(signame));

                auto response = m_client->request(Er::ProcessMgr::Requests::KillProcess, request);

                auto code = Er::getPropertyValue<Er::ProcessMgr::Props::PosixResult>(response);
                auto message = Er::getPropertyValue<Er::ProcessMgr::Props::ErrorText>(response);

                return PosixResult(code ? *code : -1, message ? std::move(*message) : "");
            }
        );
    }

private:
    std::shared_ptr<Er::Client::IClient> m_client;
    Er::Log::ILog* m_log;
};

} // namespace {}


std::unique_ptr<IProcessStub> createProcessStub(Er::Client::ChannelPtr channel, Er::Log::ILog* log)
{
    return std::make_unique<ProcessStubImpl>(channel, log);
}

} // namespace Erp::ProcessMgr {}

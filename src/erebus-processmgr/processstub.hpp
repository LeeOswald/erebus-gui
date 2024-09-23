#pragma once

#include <erebus-clt/erebus-clt.hxx>

#include "posixresult.hpp"

namespace Erp::ProcessMgr
{


struct IProcessStub
{
    virtual ~IProcessStub() {}
    virtual PosixResult kill(uint64_t pid, std::string_view signame) = 0;
};


std::unique_ptr<IProcessStub> createProcessStub(Er::Client::ChannelPtr channel, Er::Log::ILog* log);

} // namespace Erp::ProcessMgr {}

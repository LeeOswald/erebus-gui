#pragma once

#include <string>

namespace Erp::ProcessMgr
{

struct PosixResult
{
    int code = -1;
    std::string message;

    PosixResult() noexcept = default;

    template <typename MessageT>
    PosixResult(int code, MessageT&& message)
        : code(code)
        , message(std::forward<MessageT>(message))
    {}
};


} // namespace Erp::ProcessMgr {}

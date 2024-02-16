#pragma once

#include <erebus/util/stringutil.hxx>
#include <erebus-gui/erebus-gui.hpp>

#include <sstream>
#include <vector>


namespace Erc
{

namespace Private
{

class RecentEndpoints final
{
public:
    explicit RecentEndpoints(const std::string& packed)
        : m_endpoints(Er::Util::split(packed, ';'))
    {
    }

    std::string pack() const
    {
        if (m_endpoints.empty())
            return std::string();

        std::ostringstream ss;
        bool first = true;
        for (auto& ep : m_endpoints)
        {
            if (!first)
                ss << ";";
            else
                first = false;

            ss << ep;
        }

        return ss.str();
    }

    const std::vector<std::string>& all() const noexcept
    {
        return m_endpoints;
    }

    void remove(const std::string& endpoint)
    {
        auto it = std::find(m_endpoints.begin(), m_endpoints.end(), endpoint);
        if (it != m_endpoints.end())
            m_endpoints.erase(it);
    }

    void promote(const std::string& endpoint)
    {
        auto it = std::find(m_endpoints.begin(), m_endpoints.end(), endpoint);
        if (it != m_endpoints.end())
        {
            if (it == m_endpoints.begin())
                return; // already first

            m_endpoints.erase(it);
        }

        m_endpoints.insert(m_endpoints.begin(), endpoint);
    }

private:
    std::vector<std::string> m_endpoints;
};



} // namespace Private {}

} // namespace Erc {}

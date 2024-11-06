#pragma once

#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>


namespace Erp
{

namespace Client
{

class RecentEndpoints final
{
public:
    explicit RecentEndpoints(const std::string& packed, size_t limit)
        : m_limit(limit)
    {
        boost::split(m_endpoints, packed, [](char c) { return (c == ';'); });
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

        while (m_limit < m_endpoints.size())
        {
            auto it = m_endpoints.begin();
            std::advance(it, m_endpoints.size() - 1);
            Q_ASSERT(it != m_endpoints.end());
            m_endpoints.erase(it);
        }
    }

private:
    std::vector<std::string> m_endpoints;
    const size_t m_limit;
};



} // namespace Client {}

} // namespace Erp {}

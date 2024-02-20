#pragma once

#include <erebus/util/stringutil.hxx>

#include <QString>
#include <QStringList>
#include <QTextStream>


namespace Erc
{

namespace Private
{

class PluginList final
{
public:
    explicit PluginList(const QString& packed)
        : m_plugins(packed.split(';', Qt::SkipEmptyParts))
    {
    }

    explicit PluginList(const QStringList& list)
        : m_plugins(list)
    {
    }

    QString pack() const
    {
        if (m_plugins.empty())
            return QString();

        QString buffer;
        QTextStream stream(&buffer, QIODeviceBase::WriteOnly);
        bool first = true;

        for (auto& s: m_plugins)
        {
            if (!first)
                stream << ";";
            else
                first = false;

            stream << s;
        }

        stream.flush();
        return buffer;
    }

    const QStringList& all() const noexcept
    {
        return m_plugins;
    }

    void remove(const QString& plugin)
    {
        m_plugins.removeOne(plugin);
    }

    void add(const QString& plugin)
    {
        m_plugins.append(plugin);
    }

    bool exists(const QString& plugin) const noexcept
    {
        return m_plugins.contains(plugin);
    }

private:
    QStringList m_plugins;
};


} // namespace Private {}

} // namespace Erc {}

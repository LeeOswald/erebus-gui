#include "processcolumns.hpp"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

#include <span>


namespace Erp
{

namespace Private
{

void saveProcessColumns(Erc::ISettingsStorage* settings, const ProcessColumns& columns)
{
    QByteArray a;
    {
        QDataStream s(&a, QIODevice::WriteOnly);

        s << qint32(columns.size());

        for (auto const& c: columns)
        {
            s << qint32(c.id) << quint32(c.width);
        }
    }

    Erc::Option<QByteArray>::set(settings, Erp::Settings::columns, a);
}


namespace
{

ProcessColumns loadDefaultColumns(std::span<const ProcessColumnDef> columnDefs)
{
    ProcessColumns columns;

    for (auto& c: columnDefs)
    {
        if (c.type >= ProcessColumnDef::Type::Default)
        {
            columns.append(ProcessColumn(c));

            auto& column = columns.back();
            if (column.width < c.minWidth)
                column.width = c.minWidth;
        }
    }

    return columns;
}

} // namespace {}

ProcessColumns loadProcessColumns(Erc::ISettingsStorage* settings)
{
    auto a = Erc::Option<QByteArray>::get(settings, Erp::Settings::columns, QByteArray());
    if (a.isNull() || a.isEmpty())
    {
        return loadDefaultColumns(ProcessColumnDefs);
    }

    QDataStream s(&a, QIODevice::ReadOnly);
    quint32 count = 0;
    s >> count;
    if (count < 1)
    {
        return loadDefaultColumns(ProcessColumnDefs);
    }

    ProcessColumns columns;
    columns.reserve(count);

    for (quint32 index = 0; index < count; ++index)
    {
        if (s.atEnd())
            break;

        qint32 _id;
        quint32 width = 0;
        s >> _id >> width;
        if (_id >= qint32(Er::ProcessMgr::ProcessProps::PropIndices::FlagsCount))
            continue;

        auto id = unsigned(_id);

        // check if column id is valid
        auto def = std::find_if(std::begin(ProcessColumnDefs), std::end(ProcessColumnDefs), [id](const ProcessColumnDef& c) { return c.id == id; });
        if (def == std::end(ProcessColumnDefs))
            continue;

        // force min width
        if (width < def->minWidth)
            width = def->minWidth;

        // check if already loaded
        auto loaded = std::find_if(columns.begin(), columns.end(), [id](const ProcessColumn& c) { return c.id == id; });
        if (loaded != columns.end())
            loaded->width = width;
        else
            columns.append(ProcessColumn(id, def->type, def->label, width));
    }

    // append mandatory columns
    for (auto c = std::begin(ProcessColumnDefs); c != std::end(ProcessColumnDefs); ++c)
    {
        if (c->type == ProcessColumnDef::Type::Mandatory)
        {
            auto def = std::find_if(columns.begin(), columns.end(), [id = c->id](const ProcessColumn& c) { return c.id == id; });
            if (def == columns.end())
                columns.append(ProcessColumn(*c));
        }
    }

    // [Comm], [PID] & [%CPU] are hardcoded and must precede any other columns
    for (qsizetype index = 0; index < columns.size(); ++index)
    {
        if (columns[index].id == Er::ProcessMgr::ProcessProps::PropIndices::Comm)
        {
            if (index != 0)
                std::swap(columns[index], columns[0]);
        }
        else if (columns[index].id == Er::ProcessMgr::ProcessProps::PropIndices::Pid)
        {
            if (index != 1)
                std::swap(columns[index], columns[1]);
        }
        else if (columns[index].id == Er::ProcessMgr::ProcessProps::PropIndices::CpuUsage)
        {
            if (index != 2)
                std::swap(columns[index], columns[2]);
        }
    }

    return columns;
}

Er::ProcessMgr::ProcessProps::PropMask makePropMask(const ProcessColumns& columns) noexcept
{
    Er::ProcessMgr::ProcessProps::PropMask mask;

    for (auto& col: columns)
    {
        Q_ASSERT(col.id < mask.FlagsCount);
        mask.set(col.id);
    }

    return mask;
}

bool isProcessColumnsOrderSame(const ProcessColumns& prev, const ProcessColumns& current)
{
    if (prev.size() != current.size())
        return false;

    auto itPrev = prev.begin();
    auto itCurr = current.begin();

    while (itPrev != prev.end())
    {
        if (itCurr == current.end())
            return false;

        if (itCurr->id != itPrev->id)
            return false;

        ++itPrev;
        ++itCurr;
    }

    return true;
}


} // namespace Private {}

} // namespace Erp {}

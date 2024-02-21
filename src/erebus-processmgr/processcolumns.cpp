#include "processcolumns.hpp"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>


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

ProcessColumns loadDefaultColumns(const ProcessColumnDef* columnDefBegin, const ProcessColumnDef* columnDefEnd)
{
    ProcessColumns columns;

    for (auto c = columnDefBegin; c != columnDefEnd && (c->id != ProcessColumnDef::InvalidId); ++c)
    {
        if (c->type >= ProcessColumnDef::Type::Default)
        {
            columns.append(ProcessColumn(*c));

            auto& column = columns.back();
            if (column.id == Er::ProcessProps::PropIndices::Comm)
            {
                if (column.width < Erp::Settings::MinLabelColumnWidth)
                    column.width = Erp::Settings::MinLabelColumnWidth;
            }
            else
            {
                if (column.width < Erp::Settings::MinColumnWidth)
                    column.width = Erp::Settings::MinColumnWidth;
            }
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
        return loadDefaultColumns(std::begin(ProcessColumnDefs), std::end(ProcessColumnDefs));
    }

    QDataStream s(&a, QIODevice::ReadOnly);
    quint32 count = 0;
    s >> count;
    if (count < 1)
    {
        return loadDefaultColumns(std::begin(ProcessColumnDefs), std::end(ProcessColumnDefs));
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
        if (_id >= qint32(Er::ProcessProps::PropIndices::FlagsCount))
            continue;

        auto id = unsigned(_id);

        // check if column id is valid
        auto def = std::find_if(std::begin(ProcessColumnDefs), std::end(ProcessColumnDefs), [id](const ProcessColumnDef& c) { return c.id == id; });
        if (def == std::end(ProcessColumnDefs))
            continue;

        // check if already loaded
        auto loaded = std::find_if(columns.begin(), columns.end(), [id](const ProcessColumn& c) { return c.id == id; });
        if (loaded != columns.end())
            loaded->width = width;
        else
            columns.append(ProcessColumn(id, def->type, def->label, width));
    }

    // append mandatory columns
    for (auto c = std::begin(ProcessColumnDefs); c != std::end(ProcessColumnDefs) && (c->id != ProcessColumnDef::InvalidId); ++c)
    {
        if (c->type == ProcessColumnDef::Type::Mandatory)
        {
            auto def = std::find_if(std::begin(ProcessColumnDefs), std::end(ProcessColumnDefs), [id = c->id](const ProcessColumnDef& c) { return c.id == id; });
            if (def == std::end(ProcessColumnDefs))
                columns.append(ProcessColumn(*def));
        }
    }

    // force min widths
    for (auto& c: columns)
    {
        if (c.width < Settings::MinColumnWidth)
            c.width = Settings::MinColumnWidth;
    }

    // force [Name] column to be the first one
    for (qsizetype index = 0; index < columns.size(); ++index)
    {
        if (columns[index].id == Er::ProcessProps::PropIndices::Comm)
        {
            // force min [Name] width
            if (columns[index].width < Settings::MinLabelColumnWidth)
                columns[index].width = Settings::MinLabelColumnWidth;

            // force [Name] to be the first column
            if (index > 0)
                std::swap(columns[index], columns[0]);

            break;
        }
    }

    return columns;
}

Er::ProcessProps::PropMask makePropMask(const ProcessColumns& columns) noexcept
{
    Er::ProcessProps::PropMask mask;

    for (auto& col: columns)
    {
        Q_ASSERT(col.id < mask.FlagsCount);
        mask.set(col.id);
    }

    return mask;
}


} // namespace Private {}

} // namespace Erp {}

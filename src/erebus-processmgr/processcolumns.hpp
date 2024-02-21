#pragma once

#include <erebus-processmgr/processprops.hxx>

#include "settings.hpp"

#include <QString>
#include <QVector>


namespace Erp
{

namespace Private
{


struct ProcessColumnDef
{
    static constexpr unsigned InvalidId = unsigned(-1);

    enum class Type
    {
        Regular,
        Default,      // this property is present by default (but not mandatory)
        Mandatory,    // property is always present and is not displayed in [Select Columns] dialog
    };

    unsigned id;      // index in ProcessProps::PropIndices
    Type type;
    const char* label;

    constexpr ProcessColumnDef() noexcept
        : id(InvalidId)
        , type(Type::Regular)
        , label(nullptr)
    {}

    constexpr ProcessColumnDef(unsigned id, Type type, const char* label) noexcept
        : id(id)
        , type(type)
        , label(label)
    {}
};


constexpr ProcessColumnDef ProcessColumnDefs[] =
{
    ProcessColumnDef(Er::ProcessProps::PropIndices::Comm, ProcessColumnDef::Type::Mandatory, QT_TR_NOOP("Name")),
    ProcessColumnDef(Er::ProcessProps::PropIndices::Pid, ProcessColumnDef::Type::Mandatory, QT_TR_NOOP("PID")),
    ProcessColumnDef(Er::ProcessProps::PropIndices::PPid, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Parent PID")),
    ProcessColumnDef(Er::ProcessProps::PropIndices::PGrp, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Group ID")),
    ProcessColumnDef(Er::ProcessProps::PropIndices::Tpgid, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Terminal Group ID")),
    ProcessColumnDef(Er::ProcessProps::PropIndices::Session, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Session ID")),
    ProcessColumnDef(Er::ProcessProps::PropIndices::Ruid, ProcessColumnDef::Type::Default, QT_TR_NOOP("User ID")),
    ProcessColumnDef(Er::ProcessProps::PropIndices::CmdLine, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Command Line")),
    ProcessColumnDef(Er::ProcessProps::PropIndices::Exe, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Executable Name")),
    ProcessColumnDef() // null
};


struct ProcessColumn
    : public ProcessColumnDef
{
    unsigned width;

    constexpr ProcessColumn() noexcept
        : width(0)
    {}

    constexpr ProcessColumn(const ProcessColumnDef& def)
        : ProcessColumnDef(def)
        , width(def.id == Er::ProcessProps::PropIndices::Comm ? Erp::Settings::MinLabelColumnWidth : Erp::Settings::MinColumnWidth)
    {}

    constexpr ProcessColumn(unsigned id, Type type, const char* label, unsigned width) noexcept
        : ProcessColumnDef(id, type, label)
        , width(width)
    {}
};


using ProcessColumns = QVector<ProcessColumn>;


void saveProcessColumns(Erc::ISettingsStorage* settings, const ProcessColumns& columns);
ProcessColumns loadProcessColumns(Erc::ISettingsStorage* settings);

Er::ProcessProps::PropMask makePropMask(const ProcessColumns& columns) noexcept;


} // namespace Private {}

} // namespace Erp {}

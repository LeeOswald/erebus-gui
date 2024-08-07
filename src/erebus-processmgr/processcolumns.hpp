#pragma once

#include <erebus-processmgr/erebus-processmgr.hxx>

#include "settings.hpp"

#include <QString>
#include <QVector>


namespace Erp
{

namespace ProcessMgr
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
    unsigned minWidth;

    constexpr ProcessColumnDef() noexcept
        : id(InvalidId)
        , type(Type::Regular)
        , label(nullptr)
        , minWidth(30)
    {}

    constexpr ProcessColumnDef(unsigned id, Type type, const char* label, unsigned minWidth) noexcept
        : id(id)
        , type(type)
        , label(label)
        , minWidth(minWidth)
    {}
};


constexpr ProcessColumnDef ProcessColumnDefs[] =
{
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::Comm, ProcessColumnDef::Type::Mandatory, QT_TR_NOOP("Name"), 200),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::Pid, ProcessColumnDef::Type::Mandatory, QT_TR_NOOP("PID"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::CpuUsage, ProcessColumnDef::Type::Mandatory, QT_TR_NOOP("CPU"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::PPid, ProcessColumnDef::Type::Regular, QT_TR_NOOP("PPID"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::PGrp, ProcessColumnDef::Type::Regular, QT_TR_NOOP("PGRP"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::Tpgid, ProcessColumnDef::Type::Regular, QT_TR_NOOP("TPGID"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::Tty, ProcessColumnDef::Type::Regular, QT_TR_NOOP("TTY"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::Session, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Session"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::Ruid, ProcessColumnDef::Type::Default, QT_TR_NOOP("UID"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::User, ProcessColumnDef::Type::Default, QT_TR_NOOP("User"), 100),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::CmdLine, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Command Line"), 300),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::Exe, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Executable"), 300),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::StartTime, ProcessColumnDef::Type::Regular, QT_TR_NOOP("Start Time"), 100),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::State, ProcessColumnDef::Type::Default, QT_TR_NOOP("State"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::UTime, ProcessColumnDef::Type::Default, QT_TR_NOOP("UTime"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::STime, ProcessColumnDef::Type::Default, QT_TR_NOOP("STime"), 30),
    ProcessColumnDef(Er::ProcessMgr::ProcessProps::PropIndices::ThreadCount, ProcessColumnDef::Type::Default, QT_TR_NOOP("Threads"), 30),
};


struct ProcessColumn
{
    unsigned id;                   // index in ProcessProps::PropIndices
    ProcessColumnDef::Type type;
    QString label;
    unsigned width;

    constexpr ProcessColumn() noexcept
        : id(ProcessColumnDef::InvalidId)
        , type(ProcessColumnDef::Type::Regular)
        , label()
        , width(0)
    {}

    ProcessColumn(const ProcessColumnDef& def)
        : id(def.id)
        , type(def.type)
        , label(Erc::fromUtf8(def.label))
        , width(def.minWidth)
    {}

    ProcessColumn(unsigned id, ProcessColumnDef::Type type, const char* label, unsigned width) noexcept
        : id(id)
        , type(type)
        , label(Erc::fromUtf8(label))
        , width(width)
    {}
};


using ProcessColumns = QVector<ProcessColumn>;


void saveProcessColumns(Erc::ISettingsStorage* settings, const ProcessColumns& columns);
ProcessColumns loadProcessColumns(Erc::ISettingsStorage* settings);

Er::ProcessMgr::ProcessProps::PropMask makePropMask(const ProcessColumns& columns) noexcept;

bool isProcessColumnsOrderSame(const ProcessColumns& prev, const ProcessColumns& current);


} // namespace ProcessMgr {}

} // namespace Erp {}

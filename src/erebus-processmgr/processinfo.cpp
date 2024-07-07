#include "processinfo.hpp"

#include <erebus/exception.hxx>
#include <erebus-gui/erebus-gui.hpp>


namespace Erp
{

namespace Private
{

ProcessInformation::ProcessInformation(Er::PropertyBag&& bag)
    : properties(std::move(bag))
{
    // find PID (must always be present)
    this->pid = Er::getPropertyOr<Er::ProcessProps::Pid>(properties, Er::ProcessProps::Pid::ValueType(-1));
    if (this->pid == Er::ProcessProps::Pid::ValueType(-1))
        throw Er::Exception(ER_HERE(), "No PID in process properties");

    if (Er::propertyPresent<Er::ProcessProps::IsDeleted>(properties))
    {
        this->deleted = 1;
        return;
    }

    // find 'Valid' property
    this->valid = Er::getPropertyOr<Er::ProcessProps::Valid>(properties, false);
    if (!this->valid)
    {
        // maybe we've got an error message
        auto msg = Er::getProperty<Er::ProcessProps::Error>(properties);
        if (msg)
            this->error = Erc::fromUtf8(*msg);

        return;
    }

    // cache certain props
    Er::enumerateProperties(properties, [this](const Er::Property& it)
    {
        switch (it.id)
        {
        case Er::ProcessProps::IsNew::Id::value:
            this->added = std::get<bool>(it.value);
            break;

        case Er::ProcessProps::PPid::Id::value:
            this->ppid = std::get<uint64_t>(it.value);
            break;

        case Er::ProcessProps::StartTime::Id::value:
        {
            this->startTime = std::get<uint64_t>(it.value);
            Er::TimeFormatter<"%H:%M:%S %d %b %y", Er::TimeZone::Utc> fmt;
            std::ostringstream ss;
            fmt(this->startTime, ss);
            this->startTimeUtc = Erc::fromUtf8(ss.str());
            break;
        }

        case Er::ProcessProps::State::Id::value:
            this->processState = Erc::fromUtf8(std::get<std::string>(it.value));
            break;

        case Er::ProcessProps::Comm::Id::value:
            this->comm = Erc::fromUtf8(std::get<std::string>(it.value));
            break;

        case Er::ProcessProps::UTime::Id::value:
            this->uTime = std::get<double>(it.value);
            break;

        case Er::ProcessProps::STime::Id::value:
            this->sTime = std::get<double>(it.value);
            break;
        }
    });

    if (this->ppid == InvalidKey)
        this->ppid = this->pid;
}

void ProcessInformation::updateFromDiff(const ProcessInformation& diff)
{
    this->uTimePrev = this->uTime;
    this->sTimePrev = this->sTime;
    this->uTime = 0.0;
    this->sTime = 0.0;
    this->uTimeDiff = std::nullopt;
    this->sTimeDiff = std::nullopt;

    Er::enumerateProperties(diff.properties, [this, &diff](const Er::Property& diffProp)
    {
        auto myProp = Er::findProperty(this->properties, diffProp.id);
        if (!myProp)
            Er::insertProperty(this->properties, diffProp);
        else
            *myProp = diffProp;

        // update cached props if modified
        switch (diffProp.id)
        {
        case Er::ProcessProps::PPid::Id::value:
            Q_ASSERT(diff.pid != InvalidKey);
            this->ppid = diff.pid;
            break;

        case Er::ProcessProps::StartTime::Id::value:
        {
            Q_ASSERT(diff.startTime > 0);
            this->startTime = diff.startTime;
            Q_ASSERT(!diff.startTimeUtc.isEmpty());
            this->startTimeUtc = diff.startTimeUtc;
            break;
        }

        case Er::ProcessProps::State::Id::value:
            this->processState = diff.processState;
            break;

        case Er::ProcessProps::Comm::Id::value:
            this->comm = diff.comm;
            break;

        case Er::ProcessProps::UTime::Id::value:
            this->uTime = diff.uTime;
            if (this->uTimePrev > 0.0)
                this->uTimeDiff = this->uTime - this->uTimePrev;
            break;

        case Er::ProcessProps::STime::Id::value:
            this->sTime = diff.sTime;
            if (this->sTimePrev > 0.0)
                this->sTimeDiff = this->sTime - this->sTimePrev;
            break;
        }
    });

    // show process as 'running' if it has... well... run for a while since the last cycle
    if (this->processState == QLatin1String("S"))
    {
        if ((this->uTimeDiff > 0) || (this->sTimeDiff > 0))
        {
            this->processState = QLatin1String("R");
        }
    }
}


} // namespace Private {}

} // namespace Erp {}

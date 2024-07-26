#pragma once

#include <erebus-gui/settings.hpp>

#include <QSettings>

namespace Erp
{
    
namespace Client
{

class Settings final
    : public Erc::ISettingsStorage
{
public:
    QStringList keys() override
    {
        return storage().allKeys();
    }

    QVariant get(std::string_view key, const QVariant& defaultValue = QVariant()) override
    {
        return storage().value(key, defaultValue);
    }

    void set(std::string_view key, const QVariant& value) override
    {
        storage().setValue(key, value);
    }

    void sync() override
    {
        storage().sync();
    }

private:
    static QSettings& storage()
    {
        static QSettings s;
        return s;
    }
};

} // namespace Client {}

} // namespace Erp {}

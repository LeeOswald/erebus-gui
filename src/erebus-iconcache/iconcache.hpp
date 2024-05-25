#pragma once

#include <erebus/log.hxx>


#include <filesystem>


namespace ErIc
{


class IconCache final
    : public Er::NonCopyable
{
public:
    struct IconInfo
    {
        enum class Type
        {
            Valid,
            Invalid
        };

        Type type;
        std::string path;

        IconInfo(Type type) : type(type) {}
        IconInfo(Type type, std::string_view path) : type(type), path(path) {}
    };

    IconCache(Er::Log::ILog* log, std::string_view themeName, std::string_view cacheDir, std::string_view icoFormat = ".png");

    IconInfo cacheIcon(std::string_view name, unsigned size) noexcept;

private:
    IconInfo cacheIconFromAbsolutePath(std::string_view path, unsigned size) noexcept;
    IconInfo cacheIconFromName(std::string_view name, unsigned size) noexcept;
    bool createEmptyFile(const std::string& path) noexcept;

    Er::Log::ILog* m_log;
    std::string m_cacheDir;
    std::string m_icoFormat;
};



} // namespace ErIc {}

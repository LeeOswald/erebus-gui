#include "iconcache.hpp"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/posixerror.hxx>
#include <erebus-desktop/ic.hxx>

#include <sys/stat.h>

#include <QIcon>

namespace Erp
{

namespace IconCache
{


IconCache::IconCache(Er::Log::ILog* log, std::string_view themeName, std::string_view cacheDir, std::string_view icoFormat)
    : m_log(log)
    , m_cacheDir(cacheDir)
    , m_icoFormat(icoFormat)
{
    if (!themeName.empty())
        QIcon::setThemeName(QString::fromUtf8(themeName));

    ErAssert(!icoFormat.empty());
    ErAssert(icoFormat.front() == '.');

    std::filesystem::path path(m_cacheDir);
    std::error_code ec;
    if (std::filesystem::exists(path))
    {
        if (!std::filesystem::is_directory(path, ec) || ec)
            ErThrow(Er::Util::format("%s is not a directry", m_cacheDir.c_str()));

        if (::access(m_cacheDir.c_str(), R_OK | W_OK) == -1)
            ErThrow(Er::Util::format("%s is inaccessible", m_cacheDir.c_str()));
    }
    else
    {
        if (!std::filesystem::create_directory(path, ec) || ec)
            ErThrow(Er::Util::format("Failed to create %s: %s", m_cacheDir.c_str(), ec.message().c_str()));
    }
}

IconCache::IconInfo IconCache::cacheIcon(std::string_view name, unsigned size) noexcept
{
    std::filesystem::path path(name);
    if (path.is_absolute())
        return cacheIconFromAbsolutePath(name, size);
    else
        return cacheIconFromName(name, size);
}

bool IconCache::createEmptyFile(const std::string& path) noexcept
{
    auto fd = ::creat(path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        auto e = errno;
        auto msg = Er::Util::posixErrorToString(e);
        Er::Log::Error(m_log) << "Failed to create [" << path << "]: " << e << " " << msg;
        return false;
    }

    ::close(fd);
    return true;
}

IconCache::IconInfo IconCache::cacheIconFromAbsolutePath(std::string_view path, unsigned size) noexcept
{
    auto iconFilePath = Er::Desktop::makeIconCachePath(m_cacheDir, std::string(path), size, m_icoFormat);

    if (std::filesystem::exists(iconFilePath))
    {
        struct stat fs = {};
        if (::stat(iconFilePath.c_str(), &fs) == 0)
        {
            if (fs.st_size == 0)
            {
                Er::Log::Debug(m_log) << "Found NULL cached icon [" << path << "] for [" << iconFilePath << "]";
                return IconInfo(IconInfo::Type::Invalid);
            }

            Er::Log::Debug(m_log) << "Found cached icon [" << path << "] at [" << iconFilePath << "]";
            return IconInfo(IconInfo::Type::Valid, iconFilePath);
        }
    }

    std::filesystem::path sourcePath(path);
    if (!std::filesystem::exists(sourcePath))
    {
        createEmptyFile(iconFilePath);

        Er::Log::Error(m_log) << "Icon [" << path << "] does not exist";
        return IconInfo(IconInfo::Type::Invalid);
    }

    QPixmap ico;
    if (!ico.load(QString::fromUtf8(path)))
    {
        createEmptyFile(iconFilePath);

        Er::Log::Error(m_log) << "Icon [" << path << "] could not be loaded";
        return IconInfo(IconInfo::Type::Invalid);
    }

    if (!ico.save(QString::fromUtf8(iconFilePath)))
    {
        createEmptyFile(iconFilePath);

        Er::Log::Error(m_log) << "Icon [" << path << "] could not be saved to [" << iconFilePath << "]";
        return IconInfo(IconInfo::Type::Invalid);
    }

    Er::Log::Info(m_log) << "Cached icon [" << path << "] to [" << iconFilePath << "]";
    return IconInfo(IconInfo::Type::Valid, iconFilePath);
}

IconCache::IconInfo IconCache::cacheIconFromName(std::string_view name, unsigned size) noexcept
{
    auto iconFilePath = Er::Desktop::makeIconCachePath(m_cacheDir, std::string(name), size, m_icoFormat);

    if (std::filesystem::exists(iconFilePath))
    {
        struct stat fs = {};
        if (::stat(iconFilePath.c_str(), &fs) == 0)
        {
            if (fs.st_size == 0)
            {
                Er::Log::Debug(m_log) << "Found NULL cached icon [" << name << "] at [" << iconFilePath << "]";
                return IconInfo(IconInfo::Type::Invalid);
            }

            Er::Log::Debug(m_log) << "Found cached icon [" << name << "] at [" << iconFilePath << "]";
            return IconInfo(IconInfo::Type::Valid, iconFilePath);
        }
    }

    auto ico = QIcon::fromTheme(QString::fromUtf8(name));
    if (ico.isNull())
    {
        createEmptyFile(iconFilePath);

        Er::Log::Error(m_log) << "Icon [" << name << "] could not be loaded from current theme";
        return IconInfo(IconInfo::Type::Invalid);
    }

    auto pixmap = ico.pixmap(int(size));

    if (!pixmap.save(QString::fromUtf8(iconFilePath)))
    {
        createEmptyFile(iconFilePath);

        Er::Log::Error(m_log) << "Icon [" << name << "] could not be saved to [" << iconFilePath << "]";
        return IconInfo(IconInfo::Type::Invalid);
    }

    Er::Log::Info(m_log) << "Cached icon [" << name << "] to [" << iconFilePath << "]";
    return IconInfo(IconInfo::Type::Valid, iconFilePath);
}


} // namespace IconCache {}

} // namespace Erp {}

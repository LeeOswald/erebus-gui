#include <erebus/erebus.hxx>

#include <QGuiApplication>
#include <QIcon>

#include <iostream>
#include <filesystem>
#include <vector>


namespace
{

bool clearCache(const std::string& dir)
{
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);

    if (ec)
    {
        std::cerr << "Failed to clear the icon cache: " << ec.message() << "\n";
        return false;
    }

    return true;
}

bool checkCache(const std::string& dir)
{
    std::filesystem::path path(dir);
    std::error_code ec;
    if (std::filesystem::exists(path))
    {
        if (!std::filesystem::is_directory(path, ec) || ec)
        {
            std::cerr << dir << " exists but is not a directory\n";
            return false;
        }

        if (::access(dir.c_str(), R_OK | W_OK) == -1)
        {
            std::cerr << "Cache directory is inaccessible\n";
            return false;
        }

        return true;
    }

    if (!std::filesystem::create_directory(path, ec) || ec)
    {
        std::cerr << "Failed to create the cache directory: " << ec.message() << "\n";
        return false;
    }

    return true;
}

bool cacheIcon(const std::string& dir, const std::string& name, unsigned size)
{
    std::string iconFileName(name);
    iconFileName.append(std::to_string(size));
    iconFileName.append("x");
    iconFileName.append(std::to_string(size));
    iconFileName.append(".png");

    std::filesystem::path path(dir);
    path.append(iconFileName);

    std::error_code ec;
    if (std::filesystem::exists(path))
    {
        std::cout << "Found icon " << name << "\n";
        return true;
    }

    auto ico = QIcon::fromTheme(QString::fromUtf8(name));
    if (ico.isNull())
    {
        std::cerr << "No theme icon found for " << name << "\n";
        return false;
    }

    auto pixmap = ico.pixmap(int(size));
    auto destPath = path.string();
    if (!pixmap.save(QString::fromUtf8(destPath)))
    {
        std::cerr << "Failed to save " << destPath << "\n";
        return false;
    }

    std::cout << "Cached icon " << name << "\n";
    return true;
}

} // namespace {}


int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);

    std::string cacheDir;
    std::vector<std::string> requestedIcons;
    std::optional<unsigned> iconSize;

    bool clear = false;
    bool nextIsCache = false;
    bool nextIsSize = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!std::strcmp(argv[i], "--clear"))
        {
            clear = true;
        }
        else if (!std::strcmp(argv[i], "--cache"))
        {
            nextIsCache = true;
        }
        else if (nextIsCache)
        {
            nextIsCache = false;
            cacheDir = argv[i];
        }
        else if (!std::strcmp(argv[i], "--size"))
        {
            nextIsSize = true;
        }
        else if (nextIsSize)
        {
            nextIsSize = false;
            auto size = std::strtoul(argv[i], nullptr, 10);
            if (size == 0)
            {
                std::cerr << "Expected a valid icon size (--size <px>)\n";
                return EXIT_FAILURE;
            }
            iconSize = size;
        }
        else
        {
            requestedIcons.emplace_back(argv[i]);
        }
    }

    if (cacheDir.empty())
    {
        std::cerr << "Cache directory not specified (--cache <dir>)\n";
        return EXIT_FAILURE;
    }

    if (clear)
        clearCache(cacheDir);

    if (requestedIcons.empty())
    {
        if (!clear)
        {
            std::cerr << "Usage: erebus-iconcache --cache </path/to/cache> [--clear|--size <icon-size> <icon1> <icon2> ... <iconN>]\n";
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    if (!iconSize || !*iconSize)
    {
        std::cerr << "Expected a valid icon size (--size <px>)\n";
        return EXIT_FAILURE;
    }

    if (!checkCache(cacheDir))
        return EXIT_FAILURE;

    size_t succeeded = 0;
    for (auto& requested: requestedIcons)
    {
        if (cacheIcon(cacheDir, requested, *iconSize))
            ++succeeded;
    }

    return (succeeded > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}



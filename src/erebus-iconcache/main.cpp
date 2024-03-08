#include <erebus/util/sha256.hxx>
#include <erebus/util/stringutil.hxx>

#include <QGuiApplication>
#include <QIcon>

#include <fstream>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <vector>

#include <boost/program_options.hpp>

#include <fcntl.h>

namespace
{

const std::string_view DefaultIcon("application-x-executable");

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

std::string makeCachePath(const std::string& dir, const std::string& name, unsigned size)
{
    std::filesystem::path path(dir);
    auto sz = std::to_string(size);
    if (std::filesystem::path(name).is_absolute())
    {
        // make path like /tmp/iconcache/39534cecc15cd261e9eb3c8cd3544d4f839db599979a9348bf54afc896a25ce8_32x32.png
        Er::Util::Sha256 hash;
        hash.update(name);
        auto hashStr = hash.str(hash.digest());

        std::string fileName(std::move(hashStr));
        fileName.append("_");
        fileName.append(sz);
        fileName.append("x");
        fileName.append(sz);
        fileName.append(".png");

        path.append(fileName);

        return path.string();
    }

    // make path like /tmp/iconcache/mtapp_32x32.png
    std::string fileName(name);
    fileName.append("_");
    fileName.append(sz);
    fileName.append("x");
    fileName.append(sz);
    fileName.append(".png");

    path.append(fileName);

    return path.string();
}

void createEmptyFile(const std::string& path)
{
    // create an empty file to prevent further load attempts if the icon in question cannot be found
    ::close(::creat(path.c_str(), S_IRUSR | S_IWUSR));
}

bool cacheIconFromAbsolutePath(const std::string& dir, const std::string& name, unsigned size)
{
    auto iconFileName = makeCachePath(dir, name, size);
    std::filesystem::path path(iconFileName);

    if (std::filesystem::exists(path))
    {
        std::cout << "Found icon " << name << "\n";
        return true;
    }

    std::filesystem::path sourcePath(name);
    if (!std::filesystem::exists(sourcePath))
    {
        std::cerr << "Icon " << name << " does not exist\n";
        createEmptyFile(iconFileName);
        return false;
    }

    QPixmap ico;
    if (!ico.load(QString::fromUtf8(name)))
    {
        std::cerr << "Icon " << name << " could not be loaded\n";
        createEmptyFile(iconFileName);
        return false;
    }

    if (!ico.save(QString::fromUtf8(iconFileName)))
    {
        std::cerr << "Failed to save " << iconFileName << "\n";
        createEmptyFile(iconFileName);
        return false;
    }

    std::cout << "Cached icon " << name << "\n";

    return true;
}

bool cacheIcon(const std::string& dir, const std::string& name, unsigned size)
{
    auto iconFileName = makeCachePath(dir, name, size);
    std::filesystem::path path(iconFileName);

    if (std::filesystem::exists(path))
    {
        std::cout << "Found icon " << name << "\n";
        return true;
    }

    auto ico = QIcon::fromTheme(QString::fromUtf8(name));
    if (ico.isNull())
    {
        std::cerr << "No theme icon found for " << name << ": using default icon\n";

        // use default icon
        if (name != DefaultIcon)
            ico = QIcon::fromTheme(QLatin1String(DefaultIcon));

        if (ico.isNull())
        {
            std::cerr << "No theme icon found for " << DefaultIcon << "\n";
            createEmptyFile(iconFileName);
            return false;
        }
    }

    auto pixmap = ico.pixmap(int(size));

    if (!pixmap.save(QString::fromUtf8(iconFileName)))
    {
        std::cerr << "Failed to save " << iconFileName << "\n";
        createEmptyFile(iconFileName);
        return false;
    }

    std::cout << "Cached icon " << name << "\n";
    return true;
}

} // namespace {}


int main(int argc, char *argv[])
{
    try
    {
        QGuiApplication a(argc, argv);

        std::string themeName;
        std::string cacheDir;
        std::string sourceFile;
        std::string iconList;
        std::vector<std::string> requestedIcons;
        unsigned iconSize = 0;

        namespace po = boost::program_options;
        po::options_description options("Command line options");
        options.add_options()
            ("help,?", "display this message")
            ("theme", po::value<std::string>(&themeName)->default_value("hicolor"), "theme name")
            ("clear", "clear cache")
            ("cache", po::value<std::string>(&cacheDir), "icon cache directory path")
            ("source", po::value<std::string>(&sourceFile), "icon list file path")
            ("size", po::value<unsigned>(&iconSize)->default_value(16), "icon size")
            ("icons", po::value<std::string>(&iconList), "requested icon names (colon-separated)")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << options << "\n";
            return 0;
        }

        if (!iconList.empty())
            requestedIcons = Er::Util::split(iconList, std::string_view(":"), Er::Util::SplitSkipEmptyParts);

        QIcon::setThemeName(QString::fromUtf8(themeName));

        auto theme = QIcon::themeName();
        std::cout << "Theme name: " << theme.toUtf8().constData() << "\n";
        theme = QIcon::fallbackThemeName();
        std::cout << "Fallback theme name: " << theme.toUtf8().constData() << "\n";


        auto paths = QIcon::themeSearchPaths();
        for (auto& path: paths)
            std::cout << "Theme search path: " << path.toUtf8().constData() << "\n";

        paths = QIcon::fallbackSearchPaths();
        for (auto& path: paths)
            std::cout << "Fallback search path: " << path.toUtf8().constData() << "\n";

        if (!sourceFile.empty())
        {
            std::ifstream source(sourceFile);
            if (!source.is_open())
            {
                std::cerr << "Failed to open " << sourceFile << "\n";
                return -1;
            }
            else
            {
                std::string line;
                while (std::getline(source, line))
                {
                    requestedIcons.push_back(std::move(line));
                }
            }
        }

        if (cacheDir.empty())
        {
            std::cerr << "Cache directory not specified\n";
            std::cerr << options << "\n";
            return -1;
        }

        if (vm.count("clear") > 0)
        {
            clearCache(cacheDir);
        }

        if (requestedIcons.empty())
        {
            std::cout << "Nothing to do\n";
            return 0;
        }

        if (!iconSize)
        {
            std::cerr << "Expected a valid icon size\n";
            std::cerr << options << "\n";
            return -1;
        }

        if (!checkCache(cacheDir))
            return -1;

        int succeeded = 0;
        for (auto& requested: requestedIcons)
        {
            std::filesystem::path path(requested);
            if (path.is_absolute())
            {
                if (cacheIconFromAbsolutePath(cacheDir, requested, iconSize))
                    ++succeeded;
            }
            else
            {
                if (cacheIcon(cacheDir, requested, iconSize))
                    ++succeeded;
            }
        }

        return succeeded;

    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << "\n";
    }

    return -1;
}



#include "iconcache.hpp"
#include "log.hpp"
#include "server.hpp"

#include <erebus/condition.hxx>
#include <erebus/util/signalhandler.hxx>
#include <erebus/util/stringutil.hxx>

#include <QGuiApplication>

#include <iostream>

#include <boost/program_options.hpp>


namespace
{

Er::Event g_exitCondition(false);
std::optional<int> g_signalReceived;

void signalHandler(int signo)
{
    g_signalReceived = signo;
    g_exitCondition.setAndNotifyAll(true);
}


void cacheIconsFromList(Er::Log::ILog* log, const std::string& cacheDir, const std::string& themeName, const std::vector<std::string>& requested, unsigned iconSize)
{
    ErIc::IconCache cache(log, themeName, cacheDir, ".png");

    for (auto& req: requested)
    {
        cache.cacheIcon(req, iconSize);
    }
}

} // namespace {}


int main(int argc, char* argv[])
{
    // let the icon cache be accessible
    ::umask(0);

    Er::Util::SignalHandler sh({SIGINT, SIGTERM, SIGPIPE, SIGHUP});
    std::future<int> futureSigHandler =
        // spawn a thread that handles signals
        sh.asyncWaitHandler(
            [](int signo)
            {
                signalHandler(signo);
                return true;
            }
        );


    try
    {
        QGuiApplication a(argc, argv);

        std::string themeName;
        std::string cacheDir;
        std::string iconList;
        std::vector<std::string> requestedIcons;
        unsigned iconSize = 0;
        std::string mqName;

        namespace po = boost::program_options;
        po::options_description options("Command line options");
        options.add_options()
            ("help,?", "display this message")
            ("theme", po::value<std::string>(&themeName), "theme name")
            ("verbose,v", "verbose output")
            ("cache", po::value<std::string>(&cacheDir), "icon cache directory path")
            ("size", po::value<unsigned>(&iconSize)->default_value(16), "icon size")
            ("icons", po::value<std::string>(&iconList), "requested icon names (colon-separated)")
            ("queue", po::value<std::string>(&mqName), "message queue name")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << options << "\n";
            return 0;
        }

        bool verbose = (vm.count("verbose") > 0);
        ErIc::Log console(verbose ? Er::Log::Level::Debug : Er::Log::Level::Info);

        Er::LibScope er(&console);


        if (!iconList.empty())
            requestedIcons = Er::Util::split(iconList, std::string_view(":"), Er::Util::SplitSkipEmptyParts);


        if (cacheDir.empty())
        {
            std::cerr << "Cache directory not specified\n";
            std::cerr << options << "\n";
            return -1;
        }

        if (!requestedIcons.empty())
        {
            if (!iconSize)
            {
                std::cerr << "Expected a valid icon size\n";
                std::cerr << options << "\n";
                return -1;
            }

            cacheIconsFromList(&console, cacheDir, themeName, requestedIcons, iconSize);
            return 0;
        }

        if (mqName.empty())
        {
            std::cerr << "Expected a message queue name\n";
            std::cerr << options << "\n";
            return -1;
        }

        std::string mqIn(mqName);
        mqIn.append("_req");
        std::string mqOut(mqName);
        mqIn.append("_resp");

        auto ipc = Er::Desktop::createIconCacheIpc(mqIn.c_str(), mqOut.c_str(), 128);

        ErIc::IconCache cache(&console, themeName, cacheDir, ".png");

        ErIc::IconServer server(&console, &g_exitCondition, ipc, cache, 2);

        g_exitCondition.waitValue(true);

        if (g_signalReceived)
        {
            console.write(Er::Log::Level::Warning, ErLogNowhere(), "Exiting due to signal %d", *g_signalReceived);
        }

    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return -1;
    }

    return 0;
}



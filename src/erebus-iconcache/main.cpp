#include "iconcache.hpp"
#include "log.hpp"
#include "server.hpp"

#include <erebus/condition.hxx>
#include <erebus/util/signalhandler.hxx>

#include <QGuiApplication>

#include <iostream>
#include <vector>

#include <boost/algorithm/string.hpp>
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
    // let the icon cache be accessible for all users
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
            ("queue", po::value<std::string>(&mqName)->default_value("erebus"), "message queue name")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, options), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << options << "\n";
            return EXIT_SUCCESS;
        }

        bool verbose = (vm.count("verbose") > 0);
        ErIc::Log console(verbose ? Er::Log::Level::Debug : Er::Log::Level::Info);

        Er::LibScope er(&console);


        if (!iconList.empty())
            boost::split(requestedIcons, iconList, [](char c) { return (c == ':'); });


        if (cacheDir.empty())
        {
            std::cerr << "Cache directory not specified\n";
            std::cerr << options << "\n";
            return EXIT_FAILURE;
        }

        if (!requestedIcons.empty())
        {
            if (!iconSize)
            {
                std::cerr << "Expected a valid icon size\n";
                std::cerr << options << "\n";
                return EXIT_FAILURE;
            }

            cacheIconsFromList(&console, cacheDir, themeName, requestedIcons, iconSize);
            return EXIT_SUCCESS;
        }

        if (mqName.empty())
        {
            std::cerr << "Expected a message queue name\n";
            std::cerr << options << "\n";
            return EXIT_FAILURE;
        }

        std::string mqIn(mqName);
        mqIn.append("_req");
        std::string mqOut(mqName);
        mqOut.append("_resp");

        auto ipc = Er::Desktop::openIconCacheIpc(mqIn.c_str(), mqOut.c_str());
        console.write(Er::Log::Level::Info, ErLogNowhere(), "Opened message queue %s", mqIn.c_str());
        console.write(Er::Log::Level::Info, ErLogNowhere(), "Opened message queue %s", mqOut.c_str());

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
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}



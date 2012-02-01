
#include <iostream>
#include <fstream>
#include <dbglog.h>
#include <boost/optional.hpp>

#include <threadserver/configuration.h>

namespace ThreadServer {

Configuration_t::Configuration_t(int argc, char **argv)
  : nodetach(false),
    handlerNames(),
    listenerNames(),
    pidFile(),
    data()
{
    boost::program_options::options_description options("Allowed options");
    boost::program_options::variables_map variableMap;
    boost::optional<boost::program_options::parsed_options> parsedCmdLineOptions;
    boost::optional<boost::program_options::parsed_options> parsedConfigFileOptions;

    bool help;
    std::string configFile;
    std::string dbgLogFile;
    std::string dbgLogMask;
    size_t dbgLogBufSize;

    options.add_options()
        ("help,h", boost::program_options::bool_switch(&help), "produce help message")
        ("config,f", boost::program_options::value(&configFile), "set configuration file")
        ("nodetach,d,main.NoDetach", boost::program_options::bool_switch(&nodetach), "don't fork to background")
        ("main.LogFile", boost::program_options::value(&dbgLogFile), "log filename")
        ("main.LogMask", boost::program_options::value(&dbgLogMask)->default_value("I3W2E2F1"), "log mask")
        ("main.LogBufSize", boost::program_options::value(&dbgLogBufSize)->default_value(0), "log buffer size")
        ("main.Handler", boost::program_options::value(&handlerNames)->multitoken(), "handler name")
        ("main.Listener", boost::program_options::value(&listenerNames)->multitoken(), "listener name")
        ("main.PidFile", boost::program_options::value(&pidFile), "pid file")
    ;

    try {
        parsedCmdLineOptions.reset(boost::program_options::command_line_parser(
                argc, argv).options(options).allow_unregistered().run());

        boost::program_options::store(*parsedCmdLineOptions, variableMap);
    } catch (const std::exception &e) {
        std::cerr << options << std::endl;
        exit(1);
    }

    boost::program_options::notify(variableMap);

    if (help) {
        std::cerr << options << std::endl;
        exit(1);
    }

    if (!configFile.empty()) {
        LOG(INFO4, "Reading configuration from configuration file %s", configFile.c_str());

        std::ifstream configFileStream(configFile.c_str());
        if (!configFileStream.is_open()) {
            LOG(FATAL4, "Can't open configuration file %s", configFile.c_str());
            exit(1);
        }

        {
            std::set<std::string> options;
            options.insert("*");
            for (boost::program_options::detail::config_file_iterator i(configFileStream, options), e ;
                 i != e ;
                 ++i) {

                data[i->string_key].push_back(i->value[0]);
            }
        }

        configFileStream.close();
        configFileStream.open(configFile.c_str());

        try {
            parsedConfigFileOptions.reset(boost::program_options::parse_config_file(
                configFileStream, options, true));

            boost::program_options::store(*parsedConfigFileOptions, variableMap);
        } catch (const std::exception &e) {
            LOG(FATAL4, "Can't parse configuration file %s: %s", configFile.c_str(), e.what());
            exit(1);
        }

        boost::program_options::notify(variableMap);
    }

    if (!dbgLogFile.empty()) {
        logFile(dbgLogFile.c_str());
        LOG(INFO4, "Setting log file to %s", dbgLogFile.c_str());
    }

    logMask(dbgLogMask.c_str());
    LOG(INFO4, "Setting log mask to %s", dbgLogMask.c_str());

    if (dbgLogBufSize) {
        logBufSize(dbgLogBufSize);
        LOG(INFO4, "Setting log buffer size to %d", static_cast<int>(dbgLogBufSize));
    }

    if (handlerNames.empty()) {
        LOG(FATAL4, "No handlers defined");
        exit(1);
    }

    if (listenerNames.empty()) {
        LOG(FATAL4, "No listeners defined");
        exit(1);
    }
}

} // namespace ThreadServer

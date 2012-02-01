
#ifndef THREADSERVER_CONFIGURATION_H
#define THREADSERVER_CONFIGURATION_H

#include <boost/program_options.hpp>
#include <boost/program_options/detail/config_file.hpp>

#include <threadserver/error.h>

namespace ThreadServer {

class Configuration_t {
public:
    Configuration_t(int argc, char **argv);

    template<class T_t>
    T_t get(const std::string &key)
    {
        std::map<std::string, std::vector<std::string> >::const_iterator idata(data.find(key));
        if (idata == data.end()) {
            throw Error_t("Variable %s not found in configuration file", key.c_str());
        }
        return boost::lexical_cast<T_t>(idata->second[0]);
    }

    template<class T_t>
    T_t get(const std::string &key, const T_t &defaultValue)
    {
        std::map<std::string, std::vector<std::string> >::const_iterator idata(data.find(key));
        if (idata == data.end()) {
            return defaultValue;
        } else {
            return boost::lexical_cast<T_t>(idata->second[0]);
        }
    }

    template<class T_t>
    std::vector<T_t> getVector(const std::string &key)
    {
        std::vector<T_t> result;
        std::map<std::string, std::vector<std::string> >::const_iterator idata(data.find(key));
        if (idata == data.end()) {
            return result;
        }
        for (std::vector<std::string>::const_iterator ivector(idata->second.begin()) ;
             ivector != idata->second.end() ;
             ++ivector) {

            result.push_back(boost::lexical_cast<T_t>(*ivector));
        }
        return result;
    }

public:
    bool nodetach;
    std::vector<std::string> handlerNames;
    std::vector<std::string> listenerNames;
    std::string pidFile;

private:
    std::map<std::string, std::vector<std::string> > data;
};

} // namespace ThreadServer

#endif // THREADSERVER_CONFIGURATION_H

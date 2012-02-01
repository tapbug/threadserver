
#include <dbglog.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <fstream>
#include <boost/bind.hpp>

#include <threadserver/threadserver.h>
#include <threadserver/error.h>

namespace ThreadServer {

int childPid;

ThreadServer_t::ThreadServer_t(int argc, char **argv)
  : configuration(Configuration_t(argc, argv)),
    handlerMap(),
    handlerHandleSet(),
    listenerMap(),
    work(0),
    ioServiceThread(0)
{
    registerHandlers();
    registerListeners();
    if (!configuration.pidFile.empty()) {
        std::ofstream pidFileStream;
        pidFileStream.open(configuration.pidFile.c_str(), std::ios_base::out | std::ios_base::app);
        if (!pidFileStream.is_open()) {
            throw Error_t("Can't open pidfile %s", configuration.pidFile.c_str());
        }
        pidFileStream.close();
    }
    LOG(INFO4, "Server successfully initialized");
    if (!configuration.nodetach) {
        detach();
        logStderr(0);

        if (!configuration.pidFile.empty()) {
            std::ofstream pidFileStream;
            pidFileStream.open(configuration.pidFile.c_str());
            if (!pidFileStream.is_open()) {
                throw Error_t("Can't open pidfile %s", configuration.pidFile.c_str());
            }
            pidFileStream << getpid() << std::endl;
            pidFileStream.close();
        }

        for (;;) {
            {
                LOG(INFO4, "Forking from guard process");
                pid_t pid(fork());
                if (pid < 0) {
                    throw Error_t("Can't fork: %s", strerror(errno));
                }
                if (!pid) {
                    childPid = 0;
                    break;
                }
                childPid = pid;
            }
            int status;
            pid_t pid(TEMP_FAILURE_RETRY(wait(&status)));
            if (pid < 0) {
                throw Error_t("Can't wait: %s", strerror(errno));
            }
            if (WIFSIGNALED(status)) {
                int signal(WTERMSIG(status));
                if (signal != SIGKILL
                    && signal != SIGTERM
                    && signal != SIGINT) {

                    LOG(INFO4, "Server process killed with signal %s, starting it again", sys_siglist[signal]);
                    continue;
                } else {
                    LOG(INFO4, "Server process killed with signal %s, terminating", sys_siglist[signal]);
                }
            }
            exit(WEXITSTATUS(status));
        }
    } else {
        if (!configuration.pidFile.empty()) {
            std::ofstream pidFileStream;
            pidFileStream.open(configuration.pidFile.c_str());
            if (!pidFileStream.is_open()) {
                throw Error_t("Can't open pidfile %s", configuration.pidFile.c_str());
            }
            pidFileStream << getpid() << std::endl;
            pidFileStream.close();
        }
    }

    createWorkers();
}

ThreadServer_t::~ThreadServer_t()
{
    for (;;) {
        ListenerMap_t::iterator ilistenerMap(listenerMap.begin());
        if (ilistenerMap == listenerMap.end()) {
            break;
        }

        delete ilistenerMap->second;
        listenerMap.erase(ilistenerMap);
    }

    for (;;) {
        HandlerMap_t::iterator ihandlerMap(handlerMap.begin());
        if (ihandlerMap == handlerMap.end()) {
            break;
        }

        delete ihandlerMap->second;
        handlerMap.erase(ihandlerMap);
    }

    for (;;) {
        std::set<void*>::iterator ihandlerHandleSet(handlerHandleSet.begin());
        if (ihandlerHandleSet == handlerHandleSet.end()) {
            break;
        }

        dlclose(*ihandlerHandleSet);
        handlerHandleSet.erase(ihandlerHandleSet);
    }
}

void ThreadServer_t::registerHandler(Handler_t *handler)
{
    std::pair<HandlerMap_t::iterator, bool> ihandlerMap(
        handlerMap.insert(std::make_pair(handler->getName(), handler)));

    if (!ihandlerMap.second) {
        throw Error_t("Handler %s already registered", handler->getName().c_str());
    }
}

void ThreadServer_t::registerHandler(const std::string &name,
                                     const std::string &filename,
                                     const std::string &symbol,
                                     const size_t workerCount)
{
    void *handle(dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL));
    if (!handle) {
        throw Error_t("Can't load handler %s (%s): %s",
            name.c_str(), filename.c_str(), dlerror());
    }

    HandlerCreateFunction_t handlerCreateFunction(
        reinterpret_cast<HandlerCreateFunction_t>(
            dlsym(handle, symbol.c_str())));

    const char *dlsymError(dlerror());
    if (dlsymError) {
        dlclose(handle);
        throw Error_t("Can't load handler %s (%s) create function %s: %s",
            name.c_str(), filename.c_str(), symbol.c_str(), dlerror());
    }

    Handler_t *handler;
    try {
        handler = handlerCreateFunction(this, name, workerCount);
    } catch (const std::exception &e) {
        dlclose(handle);
        throw Error_t("Can't create handler %s: %s",
            name.c_str(), e.what());
    }

    registerHandler(handler);
    handlerHandleSet.insert(handle);
}

void ThreadServer_t::registerListener(Listener_t *listener)
{
    HandlerMap_t::iterator ihandlerMap(handlerMap.find(listener->getHandlerName()));
    if (ihandlerMap == handlerMap.end()) {
        throw Error_t("Handler %s not found", listener->getHandlerName().c_str());
    }

    std::pair<ListenerMap_t::iterator, bool> ilistenerMap(
        listenerMap.insert(std::make_pair(listener->getAddress(), listener)));

    if (!ilistenerMap.second) {
        throw Error_t("Listener %s already registered", listener->getAddress().c_str());
    }

    listener->setHandler(ihandlerMap->second);
}

void ThreadServer_t::registerHandlers()
{
    for (std::vector<std::string>::const_iterator ihandlerNames(configuration.handlerNames.begin()) ;
         ihandlerNames != configuration.handlerNames.end() ;
         ++ihandlerNames) {

        std::string handler(configuration.get<std::string>(*ihandlerNames + ".Handler"));
        size_t pos(handler.find(":"));
        if (pos == std::string::npos) {
            throw Error_t("Invalid handler specification %s", handler.c_str());
        }
        std::string filename(handler.substr(0, pos));
        std::string symbol(handler.substr(pos + 1));
        size_t workerCount(configuration.get<size_t>(*ihandlerNames + ".WorkerCount"));
        registerHandler(*ihandlerNames, filename, symbol, workerCount);
    }
}

void ThreadServer_t::registerListeners()
{
    for (std::vector<std::string>::const_iterator ilistenerNames(configuration.listenerNames.begin()) ;
         ilistenerNames != configuration.listenerNames.end() ;
         ++ilistenerNames) {

        std::string listenAddress(configuration.get<std::string>(*ilistenerNames + ".Address"));
        std::string handler(configuration.get<std::string>(*ilistenerNames + ".Handler"));

        LOG(INFO4, "Listener %s on %s -> %s:",
            ilistenerNames->c_str(), listenAddress.c_str(),
            handler.c_str());

        bool allowFirst(true);
        std::string order(configuration.get<std::string>(*ilistenerNames + ".Order"));
        if (order != "allow,deny") {
            if (order == "deny,allow") {
                allowFirst = false;
            } else {
                throw Error_t("Invalid Order %s", order.c_str());
            }
        }

        LOG(INFO4, "    Order = %s", order.c_str());

        std::vector<Network_t> allowed(Network_t::parse(
            configuration.getVector<std::string>(*ilistenerNames + ".Allow")));

        for (std::vector<Network_t>::const_iterator iallowed(allowed.begin()) ;
             iallowed != allowed.end() ;
             ++iallowed) {

            LOG(INFO4, "    Allow from %s/%s",
                iallowed->getAddress().to_string().c_str(), iallowed->getNetmask().to_string().c_str());
        }

        std::vector<Network_t> denied(Network_t::parse(
            configuration.getVector<std::string>(*ilistenerNames + ".Deny")));

        for (std::vector<Network_t>::const_iterator idenied(denied.begin()) ;
             idenied != denied.end() ;
             ++idenied) {

            LOG(INFO4, "    Deny from %s/%s",
                idenied->getAddress().to_string().c_str(), idenied->getNetmask().to_string().c_str());
        }

        if (allowed.empty()) {
            LOG(WARN4, "No allowed addresses for listener %s defined!",
               ilistenerNames->c_str());
        }

        registerListener(new Listener_t(
            listenAddress, handler, allowFirst, allowed, denied));
    }
}

void ThreadServer_t::run()
{
    work.reset(new boost::asio::io_service::work(ioService));

    for (ListenerMap_t::iterator ilistenerMap(listenerMap.begin()) ;
         ilistenerMap != listenerMap.end() ;
         ++ilistenerMap) {

        LOG(INFO4, "Listening on %s (handler %s)",
            ilistenerMap->second->getAddress().c_str(),
            ilistenerMap->second->getHandlerName().c_str());

        try {
            ilistenerMap->second->run(ioService);
        } catch (const std::exception &e) {
            LOG(FATAL4, "Can't listen on %s: %s",
                ilistenerMap->second->getAddress().c_str(),
                e.what());
            throw;
        }
    }

    ioServiceThread.reset(new boost::thread(boost::bind(&boost::asio::io_service::run, &ioService)));
}

void ThreadServer_t::stop()
{
    for (ListenerMap_t::iterator ilistenerMap(listenerMap.begin()) ;
         ilistenerMap != listenerMap.end() ;
         ++ilistenerMap) {

        LOG(INFO4, "Shutting down listener on %s (handler %s)",
            ilistenerMap->second->getAddress().c_str(),
            ilistenerMap->second->getHandlerName().c_str());

        ilistenerMap->second->stop();
    }

    work.reset(0);

    ioService.stop();

    ioServiceThread->join();
}

void ThreadServer_t::detach()
{
    LOG(INFO4, "Forking to background");
    pid_t firstFork = fork();
    if (firstFork < 0) {
        LOG(FATAL4, "Can't fork: %s", strerror(errno));
        exit(1);
    } else if (firstFork > 0) {
        exit(0);
    }
    if (setsid() == -1) {
        LOG(FATAL4, "Can't setsid: %s", strerror(errno));
        exit(1);
    }
    pid_t secondFork = fork();
    if (secondFork < 0) {
        LOG(FATAL4, "Can't fork: %s", strerror(errno));
        exit(1);
    } else if (secondFork > 0) {
        exit(0);
    }
    if (chdir("/") == -1) {
        LOG(FATAL4, "Can't chdir to /: %s", strerror(errno));
        exit(1);
    }
    umask(0);
    int devNull = TEMP_FAILURE_RETRY(open("/dev/null", O_RDWR));
    if (devNull >= 0) {
        if (TEMP_FAILURE_RETRY(dup2(devNull, 0)) == -1) {
            LOG(FATAL4, "Can't dup2 fd %d -> 0: %s", devNull, strerror(errno));
            exit(1);
        }
        if (TEMP_FAILURE_RETRY(dup2(devNull, 1)) == -1) {
            LOG(FATAL4, "Can't dup2 fd %d -> 1: %s", devNull, strerror(errno));
            exit(1);
        }
        if (TEMP_FAILURE_RETRY(dup2(devNull, 2)) == -1) {
            LOG(FATAL4, "Can't dup2 fd %d -> 2: %s", devNull, strerror(errno));
            exit(1);
        }
        if (devNull > 2) {
            if (TEMP_FAILURE_RETRY(close(devNull))) {
                LOG(FATAL4, "Can't close fd %d: %s", devNull, strerror(errno));
            }
        }
    } else {
        LOG(FATAL4, "Can't open /dev/null: %s", strerror(errno));
        exit(1);
    }
}

void ThreadServer_t::createWorkers()
{
    for (HandlerMap_t::iterator ihandlerMap(handlerMap.begin()) ;
         ihandlerMap != handlerMap.end() ;
         ++ihandlerMap) {

        ihandlerMap->second->createWorkers();
    }
}

boost::asio::io_service& ThreadServer_t::getIoService()
{
    return ioService;
}

}  // namespace ThreadServer

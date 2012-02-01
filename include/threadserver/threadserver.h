
#ifndef THREADSERVER_THREADSERVER_H
#define THREADSERVER_THREADSERVER_H

#include <threadserver/configuration.h>
#include <threadserver/handler.h>
#include <threadserver/listener.h>

namespace ThreadServer {

extern int childPid;

class ThreadServer_t {
public:
    ThreadServer_t(int argc, char **argv);

    ~ThreadServer_t();

    void registerHandler(const std::string &name,
                         const std::string &filename,
                         const std::string &symbol,
                         const size_t workerCount);

    void registerListener(Listener_t *listener);

    void run();

    void stop();

    boost::asio::io_service& getIoService();

private:
    typedef std::map<std::string, Handler_t*> HandlerMap_t;
    typedef std::map<std::string, Listener_t*> ListenerMap_t;
    typedef Handler_t* (*HandlerCreateFunction_t)(ThreadServer_t*, const std::string&, const size_t);

    void registerHandler(Handler_t *handler);

    void registerHandlers();

    void registerListeners();

    void detach();

    void createWorkers();

public:
    Configuration_t configuration;

private:
    HandlerMap_t handlerMap;
    std::set<void*> handlerHandleSet;
    ListenerMap_t listenerMap;
    boost::asio::io_service ioService;
    std::auto_ptr<boost::asio::io_service::work> work;
    std::auto_ptr<boost::thread> ioServiceThread;
};

} // namespace ThreadServer

#endif // THREADSERVER_THREADSERVER_H

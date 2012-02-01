
#ifndef THREADSERVER_HANDLER_DUMMYHANDLER_H
#define THREADSERVER_HANDLER_DUMMYHANDLER_H

#include <threadserver/handler.h>

namespace ThreadServer {

class ThreadServer_t;

class DummyHandler_t : public Handler_t {
public:
    class Worker_t : public Handler_t::Worker_t {
    public:
        Worker_t(DummyHandler_t *handler);

        virtual ~Worker_t();

        virtual void handle(boost::shared_ptr<SocketWork_t> socket);
    };

    DummyHandler_t(ThreadServer_t *threadServer,
                   const std::string &name,
                   const size_t workerCount);

    virtual ~DummyHandler_t();

    virtual Handler_t::Worker_t* createWorker(Handler_t *handler);
};

} // namespace ThreadServer

extern "C" {
    ThreadServer::DummyHandler_t* dummyhandler(ThreadServer::ThreadServer_t *threadServer,
                                               const std::string &name,
                                               const size_t workerCount);
}

#endif // THREADSERVER_HANDLER_DUMMYHANDLER_H


#ifndef THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_CORE_H
#define THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_CORE_H

#include <threadserver/handlers/cppfrpchandler/cppfrpchandler.h>

class Core_t {
public:
    Core_t(ThreadServer::CppFrpcHandler_t *handler);

    void threadCreate();

    void threadDestroy();

private:
    ThreadServer::CppFrpcHandler_t *handler;
};

#endif // THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_CORE_H

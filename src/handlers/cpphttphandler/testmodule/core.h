
#ifndef THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_CORE_H
#define THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_CORE_H

#include <threadserver/handlers/cpphttphandler/cpphttphandler.h>

class Core_t {
public:
    Core_t(ThreadServer::CppHttpHandler_t *handler);

    void threadCreate();

    void threadDestroy();

private:
    ThreadServer::CppHttpHandler_t *handler;
};

#endif // THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_CORE_H

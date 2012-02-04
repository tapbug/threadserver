
#ifndef THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_H
#define THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_H

#include <threadserver/handlers/cpphttphandler/cpphttphandler.h>
#include "core.h"
#include "httpinterface.h"

class TestModule_t : ThreadServer::CppHttpHandler_t::Module_t {
public:
    TestModule_t(ThreadServer::CppHttpHandler_t *handler);

    virtual ~TestModule_t();

    virtual void threadCreate();

    virtual void threadDestroy();

private:
    Core_t core;
    HttpInterface_t httpInterface;
};

extern "C" {
    TestModule_t* testmodule(ThreadServer::CppHttpHandler_t *handler);
}

#endif // THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_H


#ifndef THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_H
#define THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_H

#include <threadserver/handlers/cppfrpchandler/cppfrpchandler.h>
#include "core.h"
#include "frpcinterface.h"

class TestModule_t : ThreadServer::CppFrpcHandler_t::Module_t {
public:
    TestModule_t(ThreadServer::CppFrpcHandler_t *handler);

    virtual ~TestModule_t();

    virtual void threadCreate();

    virtual void threadDestroy();

private:
    Core_t core;
    FrpcInterface_t frpcInterface;
};

extern "C" {
    TestModule_t* testmodule(ThreadServer::CppFrpcHandler_t *handler);
}

#endif // THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_H

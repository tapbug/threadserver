
#ifndef THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_FRPCINTERFACE_H
#define THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_FRPCINTERFACE_H

#include "core.h"

#define METHOD(name) \
    FRPC::Value_t& name(FRPC::Pool_t &pool, FRPC::Array_t &params)

class FrpcInterface_t {
public:
    FrpcInterface_t(ThreadServer::CppFrpcHandler_t *handler,
                    Core_t &core);

    METHOD(testMethod);

private:
    ThreadServer::CppFrpcHandler_t *handler;
    Core_t &core;
};

#undef METHOD

#endif // THREADSERVER_HANDLER_CPP_FRPC_TESTMODULE_FRPCINTERFACE_H

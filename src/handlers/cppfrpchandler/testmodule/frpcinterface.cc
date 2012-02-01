
#include "frpcinterface.h"

#define REGMET(name, method, signature) \
    handler->registerMethod(name, ThreadServer::CppFrpcHandler_t::boundMethod(&FrpcInterface_t::method, *this), signature)

FrpcInterface_t::FrpcInterface_t(ThreadServer::CppFrpcHandler_t *handler,
                                 Core_t &core)
  : handler(handler),
    core(core)
{
    REGMET("test", testMethod, "S:");
}

#undef REGMET

#define METHOD(name) \
    FRPC::Value_t& FrpcInterface_t::name(FRPC::Pool_t &pool, FRPC::Array_t &params)

METHOD(testMethod)
{
    return pool.Struct(
        "status", pool.Int(200),
        "statusMessage", pool.String("OK"),
        "test", pool.String("Hello World!"),
        "clientAddress", pool.String(handler->getWork()->getClientAddress()));
}

#undef METHOD

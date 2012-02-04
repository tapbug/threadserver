
#ifndef THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_HTTPINTERFACE_H
#define THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_HTTPINTERFACE_H

#include <threadserver/handlers/cpphttphandler/json.h>

#include "core.h"

#define METHOD(name) \
    ThreadServer::JSON::Value_t& name(ThreadServer::JSON::Pool_t &pool, \
                                      const ThreadServer::CppHttpHandler_t::Request_t &request, \
                                      ThreadServer::CppHttpHandler_t::Response_t &response, \
                                      const ThreadServer::CppHttpHandler_t::Parameters_t &params)

class HttpInterface_t {
public:
    HttpInterface_t(ThreadServer::CppHttpHandler_t *handler,
                    Core_t &core);

    METHOD(testMethod);

private:
    ThreadServer::CppHttpHandler_t *handler;
    Core_t &core;
};

#undef METHOD

#endif // THREADSERVER_HANDLER_CPP_HTTP_TESTMODULE_HTTPINTERFACE_H

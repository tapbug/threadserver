
#include "httpinterface.h"

#define REGMET(name, method) \
    handler->registerMethod(name, ThreadServer::CppHttpHandler_t::jsonMethod(&HttpInterface_t::method, *this))

HttpInterface_t::HttpInterface_t(ThreadServer::CppHttpHandler_t *handler,
                                 Core_t &core)
  : handler(handler),
    core(core)
{
    REGMET("/test", testMethod);
}

#undef REGMET

#define METHOD(name) \
    ThreadServer::JSON::Value_t& HttpInterface_t::name(ThreadServer::JSON::Pool_t &pool, \
                                                       const ThreadServer::CppHttpHandler_t::Request_t &request, \
                                                       ThreadServer::CppHttpHandler_t::Response_t &response, \
                                                       const ThreadServer::CppHttpHandler_t::Parameters_t &params)

METHOD(testMethod)
{
    boost::optional<std::string> test(
        params.getFirst<std::string>("test"));

    ThreadServer::JSON::Struct_t &result(pool.Struct().append(
        "string", pool.String("String")).append(
        "int", pool.Int(1)).append(
        "double", pool.Double(3.14926)).append(
        "null", pool.Null()).append(
        "true", pool.Bool(true)).append(
        "false", pool.Bool(false)).append(
        "array", pool.Array().push_back(
            pool.String("Hello")).push_back(
            pool.String("World!"))));

    if (test) {
        result.append(
            "test", pool.String(*test));
    }

    return result;
}

#undef METHOD


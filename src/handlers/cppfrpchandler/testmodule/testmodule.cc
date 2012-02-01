
#include <dbglog.h>
#include "testmodule.h"

TestModule_t::TestModule_t(ThreadServer::CppFrpcHandler_t *handler)
  : ThreadServer::CppFrpcHandler_t::Module_t(handler),
    core(handler),
    frpcInterface(handler, core)
{
}

TestModule_t::~TestModule_t()
{
    LOG(INFO4, "Destroying test module");
}

void TestModule_t::threadCreate()
{
    LOG(INFO4, "Creating test module thread specific stuff");
    core.threadCreate();
}

void TestModule_t::threadDestroy()
{
    LOG(INFO4, "Destroying test module thread specific stuff");
    core.threadDestroy();
}

extern "C" {
    TestModule_t* testmodule(ThreadServer::CppFrpcHandler_t *handler)
    {
        return new TestModule_t(handler);
    }
}

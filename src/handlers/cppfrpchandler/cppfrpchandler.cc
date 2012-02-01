
#include <dbglog.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <fstream>

#include <threadserver/threadserver.h>
#include <threadserver/error.h>
#include <threadserver/handlers/cppfrpchandler/cppfrpchandler.h>

namespace ThreadServer {

CppFrpcHandler_t::CppFrpcHandler_t(ThreadServer_t *threadServer,
                                   const std::string &name,
                                   const size_t workerCount)
  : Handler_t(threadServer, name, workerCount),
    moduleHandle(0),
    module(0),
    callbacks(0),
    frpcConfig(0),
    frpc(0),
    work(0),
    helpDirectory(threadServer->configuration.get<std::string>(
        name + ".HelpDirectory", ""))
{
    std::string module(threadServer->configuration.get<std::string>(name + ".Module"));
    size_t pos(module.find(":"));
    if (pos == std::string::npos) {
        throw Error_t("Invalid module specification %s", module.c_str());
    }
    std::string filename(module.substr(0, pos));
    std::string symbol(module.substr(pos + 1));

    loadModule(filename, symbol);

    LOG(INFO4, "CppFrpcHandler module=%s", module.c_str());
}

CppFrpcHandler_t::~CppFrpcHandler_t()
{
    destroyWorkers();
}

Handler_t::Worker_t* CppFrpcHandler_t::createWorker(Handler_t *handler)
{
    return new Worker_t(dynamic_cast<CppFrpcHandler_t*>(handler));
}

CppFrpcHandler_t::Worker_t::Worker_t(CppFrpcHandler_t *handler)
  : Handler_t::Worker_t(handler),
    handler(handler)
{
    handler->callbacks.reset(new Callbacks_t());

    handler->frpcConfig.reset(new FRPC::Server_t::Config_t(
        10000, 10000, true, 100, true, handler->callbacks.get()));

    handler->frpc.reset(new FRPC::Server_t(*handler->frpcConfig));

    handler->module->threadCreate();
}

CppFrpcHandler_t::Worker_t::~Worker_t()
{
    handler->module->threadDestroy();

    delete handler->frpc.release();
    delete handler->frpcConfig.release();
    delete handler->callbacks.release();
}

void CppFrpcHandler_t::Worker_t::handle(boost::shared_ptr<SocketWork_t> socket)
{
    if (socket->forbidden) {
        handler->forbidden(socket);
        return;
    }

    handler->module->_headersIn.reset(new FRPC::HTTPHeader_t());
    handler->module->_headersOut.reset(new FRPC::HTTPHeader_t());
    handler->work.reset(socket.get());
    try {
        handler->frpc->serve(handler->work->getSocket()->native(),
                             0,
                             *handler->module->_headersIn,
                             *handler->module->_headersOut);
    } catch (...) {
        handler->module->_headersIn.reset();
        handler->module->_headersOut.reset();
        handler->work.release();
        throw;
    }
    handler->module->_headersIn.reset();
    handler->module->_headersOut.reset();
    handler->work.release();
}

void CppFrpcHandler_t::Callbacks_t::preRead()
{
}

void CppFrpcHandler_t::Callbacks_t::preProcess(const std::string &methodName,
                                               const std::string &clientIP,
                                               FRPC::Array_t &params)
{
    std::string str;
    dumpFastrpcTree(params, str, 2);
    LOG(INFO2,
        "Calling method %s(%s) from IP: %s",
        methodName.c_str(),
        str.c_str(),
        clientIP.c_str());
}

void CppFrpcHandler_t::Callbacks_t::postProcess(const std::string &methodName,
                                                const std::string &clientIP,
                                                const FRPC::Array_t &params,
                                                const FRPC::Value_t &result,
                                                const FRPC::MethodRegistry_t::TimeDiff_t &time)
{
    std::string str;
    dumpFastrpcTree(result, str, 2);
    LOG(INFO2,
        "Method: %s returned %s after %ld seconds and %ld microseconds",
        methodName.c_str(), str.c_str(),
        static_cast<long>(time.second),
        static_cast<long>(time.usecond));
}

void CppFrpcHandler_t::Callbacks_t::postProcess(const std::string &methodName,
                                                const std::string &clientIP,
                                                const FRPC::Array_t &params,
                                                const FRPC::Fault_t &fault,
                                                const FRPC::MethodRegistry_t::TimeDiff_t &time)
{
    LOG(WARN1,
        "Method: %s returned fault (%d %s) after %ld secondes and %ld microseconds",
        methodName.c_str(), fault.errorNum(), fault.message().c_str(),
        static_cast<long>(time.second), static_cast<long>(time.usecond));
}

CppFrpcHandler_t::DlHandleGuard_t::DlHandleGuard_t(void *handle)
  : handle(handle)
{
}

CppFrpcHandler_t::DlHandleGuard_t::~DlHandleGuard_t()
{
    if (handle) {
        dlclose(handle);
        handle = 0;
    }
}

void CppFrpcHandler_t::DlHandleGuard_t::reset(void *_handle)
{
    if (handle) {
        dlclose(handle);
    }
    handle = _handle;
}

void* CppFrpcHandler_t::DlHandleGuard_t::get()
{
    return handle;
}

#define RPCERROR_MESSAGEBUFFERSIZE 65536

CppFrpcHandler_t::RpcError_t::RpcError_t(const int _code,
                                         const char *format, ...)
  : Error_t(({
        va_list valist;
        char messageBuffer[RPCERROR_MESSAGEBUFFERSIZE];
        va_start(valist, format);
        vsnprintf(messageBuffer, RPCERROR_MESSAGEBUFFERSIZE, format, valist);
        va_end(valist);
        std::string result(messageBuffer);
        result;
    }).c_str()),
    _code(_code)
{
}

CppFrpcHandler_t::RpcError_t::RpcError_t(const int _code,
                                         const std::string &message)
  : Error_t(message),
    _code(_code)
{
}

int CppFrpcHandler_t::RpcError_t::code() const
{
    return _code;
}

#undef RPCERROR_MESSAGEBUFFERSIZE

void CppFrpcHandler_t::loadModule(const std::string &filename,
                                  const std::string &symbol)
{
    moduleHandle.reset(dlopen(filename.c_str(), RTLD_LAZY | RTLD_LOCAL));
    if (!moduleHandle.get()) {
        throw Error_t("Can't load module %s: %s",
            filename.c_str(), dlerror());
    }

    ModuleCreateFunction_t moduleCreateFunction(
        reinterpret_cast<ModuleCreateFunction_t>(
            dlsym(moduleHandle.get(), symbol.c_str())));

    const char *dlsymError(dlerror());
    if (dlsymError) {
        throw Error_t("Can't load module %s create function %s: %s",
            filename.c_str(), symbol.c_str(), dlerror());
    }

    try {
        module.reset(moduleCreateFunction(this));
    } catch (const std::exception &e) {
        throw Error_t("Can't create module %s: %s",
            filename.c_str(), e.what());
    }
}

namespace {

class HttpIo_t : public FRPC::HTTPIO_t {
public:
    HttpIo_t(int fd, int readTimeout, int writeTimeout, int lineSizeLimit, int bodySizeLimit)
      : FRPC::HTTPIO_t(fd, readTimeout, writeTimeout, lineSizeLimit, bodySizeLimit)
    {
    }

    ~HttpIo_t()
    {
        setSocket(-1);
    }
};

}

void CppFrpcHandler_t::forbidden(boost::shared_ptr<SocketWork_t> socket)
{
    HttpIo_t io(socket->getSocket()->native(), 10000, 10000, 1024, 1024);

    try {
        std::string requestLine(io.readLine());
        std::vector<std::string> request(io.splitBySpace(requestLine, 3));
        if (request.size() != 3) {
            throw Error_t("Bad HTTP request: %s",
                requestLine.substr(0, 30).c_str());
        }

        if (request[2] != "HTTP/1.0" && request[2] != "HTTP/1.1") {
            throw Error_t(
                "Bad HTTP protocol version or type: %s",
                request[2].c_str());
        }

        FRPC::HTTPHeader_t header;
        io.readHeader(header);
        std::string data(
            " 403 Forbidden\r\n"
            "Accept: text/xml, application/x-frpc\r\n"
            "Server: ThreadServer/CppFrpcHandler Linux\r\n\r\n");
        data = request[2] + data;
        io.sendData(data.c_str(), data.size(), false);
    } catch (...) {
        std::string data(
            "HTTP/1.0 400 Bad Request\r\n"
            "Accept: text/xml, application/x-frpc\r\n"
            "Server: ThreadServer/CppFrpcHandler Linux\r\n\r\n");
        try {
            io.sendData(data.c_str(), data.size(), false);
        } catch (const std::exception &e) {
            throw Error_t("Can't send data: %s", e.what());
        }
    }
}

void CppFrpcHandler_t::registerMethod(const std::string &methodName,
                                      FRPC::Method_t *method,
                                      const std::string &signature)
{
    frpc->registry().registerMethod(
        methodName, method, signature, loadHelp(methodName));
}

std::string CppFrpcHandler_t::loadHelp(const std::string &methodName) const
{
    if (helpDirectory.empty()) {
        return "No help directory given";
    }
    std::string filename(helpDirectory + "/" + methodName);
    std::ifstream helpStream(filename.c_str());
    if (!helpStream.is_open()) {
        throw Error_t("Can't open help file %s", filename.c_str());
    }
    helpStream.seekg(0, std::ios::end);
    size_t size(helpStream.tellg());
    helpStream.seekg(0, std::ios::beg);
    char buffer[size];
    helpStream.read(buffer, size);
    helpStream.close();
    return std::string(buffer, size);
}

SocketWork_t* CppFrpcHandler_t::getWork()
{
    return work.get();
}

CppFrpcHandler_t::Module_t::Module_t(CppFrpcHandler_t *handler)
  : handler(handler),
    _headersIn(),
    _headersOut()
{
}

CppFrpcHandler_t::Module_t::~Module_t()
{
}

void CppFrpcHandler_t::Module_t::threadCreate()
{
}

void CppFrpcHandler_t::Module_t::threadDestroy()
{
}

const FRPC::HTTPHeader_t& CppFrpcHandler_t::Module_t::headersIn() const
{
    return *_headersIn;
}

FRPC::HTTPHeader_t& CppFrpcHandler_t::Module_t::headersOut()
{
    return *_headersOut;
}

} // namespace ThreadServer

extern "C" {
    ThreadServer::CppFrpcHandler_t* cppfrpchandler(ThreadServer::ThreadServer_t *threadServer,
                                                   const std::string &name,
                                                   const size_t workerCount)
    {
        return new ThreadServer::CppFrpcHandler_t(threadServer, name, workerCount);
    }
}

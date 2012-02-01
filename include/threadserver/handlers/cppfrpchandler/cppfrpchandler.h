
#ifndef THREADSERVER_HANDLER_CPP_FRPC_H
#define THREADSERVER_HANDLER_CPP_FRPC_H

#include <set>
#include <frpc.h>
#include <frpcfault.h>
#include <frpcmethod.h>
#include <frpcmethodregistry.h>
#include <frpcserver.h>
#include <frpcvalue.h>
#include <threadserver/error.h>
#include <threadserver/handler.h>
#include <boost/thread/tss.hpp>

namespace ThreadServer {

class ThreadServer_t;

class CppFrpcHandler_t : public Handler_t {
friend class Worker_t;
public:
    class Worker_t : public Handler_t::Worker_t {
    public:
        Worker_t(CppFrpcHandler_t *handler);

        virtual ~Worker_t();

        virtual void handle(boost::shared_ptr<SocketWork_t> socket);

    private:
        CppFrpcHandler_t *handler;
    };

    class Callbacks_t : public FRPC::MethodRegistry_t::Callbacks_t {
    private:
        virtual void preRead();

        virtual void preProcess(const std::string &methodName,
                                const std::string &clientIP,
                                FRPC::Array_t &params);

        virtual void postProcess(const std::string &methodName,
                                 const std::string &clientIP,
                                 const FRPC::Array_t &params,
                                 const FRPC::Value_t &result,
                                 const FRPC::MethodRegistry_t::TimeDiff_t &time);

        virtual void postProcess(const std::string &methodName,
                                 const std::string &clientIP,
                                 const FRPC::Array_t &params,
                                 const FRPC::Fault_t &fault,
                                 const FRPC::MethodRegistry_t::TimeDiff_t &time);
    };

    class Module_t {
    friend class CppFrpcHandler_t;
    public:
        Module_t(CppFrpcHandler_t *handler);

        virtual ~Module_t();

        virtual void threadCreate();

        virtual void threadDestroy();

        const FRPC::HTTPHeader_t& headersIn() const;

        FRPC::HTTPHeader_t& headersOut();

    protected:
        CppFrpcHandler_t *handler;
        boost::thread_specific_ptr<FRPC::HTTPHeader_t> _headersIn;
        boost::thread_specific_ptr<FRPC::HTTPHeader_t> _headersOut;
    };

    class RpcError_t : public Error_t {
    public:
        RpcError_t(const int _code, const char *format, ...);

        RpcError_t(const int _code, const std::string &message);

        int code() const;

    private:
        int _code;
    };

    template<class Object_t>
    class BoundMethod_t : public FRPC::Method_t {
    public:
        typedef FRPC::Value_t& (Object_t::*Handler_t)(FRPC::Pool_t &pool, FRPC::Array_t &params);

        BoundMethod_t(Object_t &object, Handler_t handler)
          : Method_t(),
            object(object),
            handler(handler)
        {
        }

        virtual ~BoundMethod_t()
        {
        }

        virtual FRPC::Value_t& call(FRPC::Pool_t& pool, FRPC::Array_t& params)
        {
            try {
                return (object.*handler)(pool, params);
            } catch (const RpcError_t &e) {
                return pool.Struct(
                    "status", pool.Int(e.code()),
                    "statusMessage", pool.String(e.what()));
            } catch (const std::exception &e) {
                return pool.Struct(
                    "status", pool.Int(500),
                    "statusMessage", pool.String(e.what()));
            }
        }

    private:
        Object_t &object;
        Handler_t handler;
    };

    template<class Object_t>
    static BoundMethod_t<Object_t>* boundMethod(typename BoundMethod_t<Object_t>::Handler_t handler, Object_t &object)
    {
        return new BoundMethod_t<Object_t>(object, handler);
    }

    CppFrpcHandler_t(ThreadServer_t *threadServer,
                     const std::string &name,
                     const size_t workerCount);

    virtual ~CppFrpcHandler_t();

    virtual Handler_t::Worker_t* createWorker(Handler_t *handler);

    void loadModule(const std::string &filename,
                    const std::string &symbol);

    void forbidden(boost::shared_ptr<SocketWork_t> socket);

    void registerMethod(const std::string &methodName,
                        FRPC::Method_t *method,
                        const std::string &signature = "");

    SocketWork_t* getWork();

private:
    typedef Module_t* (*ModuleCreateFunction_t)(CppFrpcHandler_t*);

    class DlHandleGuard_t {
    public:
        DlHandleGuard_t(void *handle = 0);

        ~DlHandleGuard_t();

        void reset(void *_handle);

        void* get();

    private:
        void *handle;
    };

    std::string loadHelp(const std::string &methodName) const;

    DlHandleGuard_t moduleHandle;
    std::auto_ptr<Module_t> module;
    boost::thread_specific_ptr<Callbacks_t> callbacks;
    boost::thread_specific_ptr<FRPC::Server_t::Config_t> frpcConfig;
    boost::thread_specific_ptr<FRPC::Server_t> frpc;
    boost::thread_specific_ptr<SocketWork_t> work;
    std::string helpDirectory;
};

} // namespace ThreadServer

extern "C" {
    ThreadServer::CppFrpcHandler_t* cppfrpchandler(ThreadServer::ThreadServer_t *threadServer,
                                                   const std::string &name,
                                                   const size_t workerCount);
}

#endif // THREADSERVER_HANDLER_CPP_FRPC_H

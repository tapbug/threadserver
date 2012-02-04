
#ifndef THREADSERVER_HANDLER_CPP_HTTP_H
#define THREADSERVER_HANDLER_CPP_HTTP_H

#include <set>
#include <dbglog.h>
#include <threadserver/error.h>
#include <threadserver/handler.h>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <boost/thread/tss.hpp>
#include <frpchttp.h>

#include "json.h"

namespace ThreadServer {

class ThreadServer_t;

class CppHttpHandler_t : public Handler_t {
friend class Worker_t;
public:
    class Worker_t : public Handler_t::Worker_t {
    public:
        Worker_t(CppHttpHandler_t *handler);

        virtual ~Worker_t();

        virtual void handle(boost::shared_ptr<SocketWork_t> socket);

    private:
        CppHttpHandler_t *handler;
    };

    class Module_t {
    friend class CppHttpHandler_t;
    public:
        Module_t(CppHttpHandler_t *handler);

        virtual ~Module_t();

        virtual void threadCreate();

        virtual void threadDestroy();

    protected:
        CppHttpHandler_t *handler;
    };

    class HttpError_t : public Error_t {
    public:
        HttpError_t(const int _code, const char *format, ...);

        HttpError_t(const int _code, const std::string &message);

        int code() const;

    private:
        int _code;
    };

    class Message_t {
    public:
        Message_t(const std::string &protocol = "HTTP/1.0");

        int status;
        std::string protocol;
        FRPC::HTTPHeader_t headers;
        std::string contentType;
        std::string data;
    };

    class Request_t : public Message_t {
    public:
        Request_t();

        std::string method;
        std::string unparsedUri;
        std::string uri;
        std::vector<std::string> matchGroups;
    };

    class Response_t : public Message_t {
    public:
        Response_t(const Request_t &request);

        std::string statusMessage;
        std::string debugLogInfo;
        bool dontLog;
    };

    class Parameters_t {
    public:
        typedef std::map<std::string, std::list<std::string> > Data_t;

        Parameters_t();

        void parse(const std::string &params);

        template<class T_t>
        boost::optional<T_t> getFirst(const std::string &name) const
        {
            Data_t::const_iterator idata(data.find(name));
            if (idata == data.end()) {
                return boost::optional<T_t>();
            } else {
                return boost::optional<T_t>(boost::lexical_cast<T_t>(idata->second.front()));
            }
        }

        template<class T_t>
        std::list<T_t> get(const std::string &name) const
        {
            std::list<T_t> result;
            Data_t::const_iterator idata(data.find(name));
            if (idata == data.end()) {
                return std::list<T_t>();
            } else {
                for (std::list<std::string>::const_iterator ilist(idata->second.begin()) ;
                     ilist != idata->second.end() ;
                     ++ilist) {

                    result.push_back(boost::lexical_cast<T_t>(*ilist));
                }
                return result;
            }
        }

    protected:
        int unhex(const char c);

        std::string unescape(const std::string &s);

        Data_t data;
    };

    class Method_t {
    public:
        Method_t();

        virtual ~Method_t();

        virtual void call(const Request_t &request, Response_t &response) = 0;
    };

    template<class Object_t>
    class BoundMethod_t : public Method_t {
    public:
        typedef void (Object_t::*Handler_t)(const Request_t &request, Response_t &response);

        BoundMethod_t(Object_t &object, Handler_t handler)
          : Method_t(),
            object(object),
            handler(handler)
        {
        }

        virtual ~BoundMethod_t()
        {
        }

        virtual void call(const Request_t &request, Response_t &response)
        {
            return (object.*handler)(request, response);
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

    template<class Object_t>
    class HttpMethod_t : public Method_t {
    public:
        typedef void (Object_t::*Handler_t)(const Request_t &request,
                                            Response_t &response,
                                            const Parameters_t &parameters);

        HttpMethod_t(Object_t &object, Handler_t handler)
          : Method_t(),
            object(object),
            handler(handler)
        {
        }

        virtual ~HttpMethod_t()
        {
        }

        virtual void call(const Request_t &request, Response_t &response)
        {
            Parameters_t params;
            size_t pos(request.unparsedUri.find("?"));
            size_t pos2(request.unparsedUri.find("#"));
            if (pos != std::string::npos) {
                if (pos2 != std::string::npos) {
                    if (pos2 > pos) {
                        params.parse(request.unparsedUri.substr(pos+1, pos2));
                    }
                } else {
                    params.parse(request.unparsedUri.substr(pos+1));
                }
            }
            if (request.method == "POST") {
                params.parse(request.data);
            }
            try {
                (object.*handler)(request, response, params);
            } catch (const HttpError_t &e) {
                if (e.code() / 100 >= 4) {
                    throw e;
                } else {
                    response.status = e.code();
                }
            }
        }

    private:
        Object_t &object;
        Handler_t handler;
    };

    template<class Object_t>
    static HttpMethod_t<Object_t>* httpMethod(typename HttpMethod_t<Object_t>::Handler_t handler, Object_t &object)
    {
        return new HttpMethod_t<Object_t>(object, handler);
    }

    template<class Object_t>
    class JsonMethod_t : public Method_t {
    public:
        typedef JSON::Value_t& (Object_t::*Handler_t)(JSON::Pool_t &pool,
                                                      const Request_t &request,
                                                      Response_t &response,
                                                      const Parameters_t &parameters);

        JsonMethod_t(Object_t &object, Handler_t handler)
          : Method_t(),
            object(object),
            handler(handler)
        {
        }

        virtual ~JsonMethod_t()
        {
        }

        virtual void call(const Request_t &request, Response_t &response)
        {
            Parameters_t params;
            size_t pos(request.unparsedUri.find("?"));
            size_t pos2(request.unparsedUri.find("#"));
            if (pos != std::string::npos) {
                if (pos2 != std::string::npos) {
                    if (pos2 > pos) {
                        params.parse(request.unparsedUri.substr(pos+1, pos2));
                    }
                } else {
                    params.parse(request.unparsedUri.substr(pos+1));
                }
            }
            if (request.method == "POST") {
                params.parse(request.data);
            }
            response.contentType = "application/json; charset=utf-8";
            JSON::Pool_t pool;
            try {
                JSON::Value_t &result((object.*handler)(pool, request, response, params));
                response.data = std::string(result);
                if (logCheckLevel(DBG1)) {
                    if (response.debugLogInfo.empty()) {
                        LOG(DBG1, "Response:\n%s\n",
                            response.data.c_str());
                    } else {
                        LOG(DBG1, "[%s] Response:\n%s\n",
                            response.debugLogInfo.c_str(), response.data.c_str());
                    }
                }
            } catch (const HttpError_t &e) {
                if (e.code() / 100 >= 4) {
                    throw e;
                } else {
                    response.status = e.code();
                }
            }
        }

    private:
        Object_t &object;
        Handler_t handler;
    };

    template<class Object_t>
    static JsonMethod_t<Object_t>* jsonMethod(typename JsonMethod_t<Object_t>::Handler_t handler, Object_t &object)
    {
        return new JsonMethod_t<Object_t>(object, handler);
    }

    CppHttpHandler_t(ThreadServer_t *threadServer,
                     const std::string &name,
                     const size_t workerCount);

    virtual ~CppHttpHandler_t();

    virtual Handler_t::Worker_t* createWorker(Handler_t *handler);

    void loadModule(const std::string &filename,
                    const std::string &symbol);

    void forbidden(boost::shared_ptr<SocketWork_t> socket);

    void registerMethod(const std::string &location,
                        Method_t *method);

    SocketWork_t* getWork();

private:
    typedef Module_t* (*ModuleCreateFunction_t)(CppHttpHandler_t*);

    class DlHandleGuard_t {
    public:
        DlHandleGuard_t(void *handle = 0);

        ~DlHandleGuard_t();

        void reset(void *_handle);

        void* get();

    private:
        void *handle;
    };

    DlHandleGuard_t moduleHandle;
    std::auto_ptr<Module_t> module;
    boost::thread_specific_ptr<SocketWork_t> work;
    int maxRequestSize;
    std::list<std::pair<boost::regex, Method_t*> > methodRegistry;
};

} // namespace ThreadServer

extern "C" {
    ThreadServer::CppHttpHandler_t* cpphttphandler(ThreadServer::ThreadServer_t *threadServer,
                                                   const std::string &name,
                                                   const size_t workerCount);
}

#endif // THREADSERVER_HANDLER_CPP_HTTP_H

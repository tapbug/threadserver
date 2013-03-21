
#include <dbglog.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <fstream>

#include <frpchttpclient.h>
#include <frpchttpio.h>
#include <frpcunmarshaller.h>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lambda/lambda.hpp>

#include <threadserver/threadserver.h>
#include <threadserver/error.h>
#include <threadserver/handlers/cpphttphandler/cpphttphandler.h>

#include <mimetic/mimetic.h>

namespace ThreadServer {

CppHttpHandler_t::CppHttpHandler_t(ThreadServer_t *threadServer,
                                   const std::string &name,
                                   const size_t workerCount)
  : Handler_t(threadServer, name, workerCount),
    moduleHandle(0),
    module(0),
    work(0),
    readTimeout(threadServer->configuration.get<time_t>(name + ".ReadTimeout", 10000)),
    writeTimeout(threadServer->configuration.get<time_t>(name + ".WriteTimeout", 10000)),
    maxLineSize(threadServer->configuration.get<size_t>(name + ".MaxLineSize", 1024)),
    maxRequestSize(threadServer->configuration.get<int>(name + ".MaxRequestSize", 1024*1024)),
    methodRegistry(0)
{
    std::string module(threadServer->configuration.get<std::string>(name + ".Module"));
    size_t pos(module.find(":"));
    if (pos == std::string::npos) {
        throw Error_t("Invalid module specification %s", module.c_str());
    }
    std::string filename(module.substr(0, pos));
    std::string symbol(module.substr(pos + 1));

    loadModule(filename, symbol);

    LOG(INFO4, "CppHttpHandler module=%s", module.c_str());
}

CppHttpHandler_t::~CppHttpHandler_t()
{
    destroyWorkers();
}

Handler_t::Worker_t* CppHttpHandler_t::createWorker(Handler_t *handler)
{
    return new Worker_t(dynamic_cast<CppHttpHandler_t*>(handler));
}

CppHttpHandler_t::Worker_t::Worker_t(CppHttpHandler_t *handler)
  : Handler_t::Worker_t(handler),
    handler(handler)
{
    handler->methodRegistry.reset(
        new std::list<std::pair<boost::regex, Method_t*> >());

    handler->module->threadCreate();
}

CppHttpHandler_t::Worker_t::~Worker_t()
{
    handler->module->threadDestroy();

    delete handler->methodRegistry.release();
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

class Reader_t : public FRPC::UnMarshaller_t {
public:
    Reader_t(std::string &data)
      : data(data)
    {
    }

    virtual void unMarshall(const char *dataPart, unsigned int size, char)
    {
        data += std::string(dataPart, size);
    }

    virtual void finish()
    {
    }

    std::string &data;
};

}

void CppHttpHandler_t::Worker_t::handle(boost::shared_ptr<SocketWork_t> socket)
{
    if (socket->forbidden) {
        handler->forbidden(socket);
        return;
    }

    handler->work.reset(socket.get());
    try {
        HttpIo_t io(
            socket->getSocket()->native(),
            handler->readTimeout,
            handler->writeTimeout,
            handler->maxLineSize,
            handler->maxRequestSize);

        Request_t request;

        std::string requestLine;

        try {
            requestLine = io.readLine();
            std::vector<std::string> splittedRequestLine(io.splitBySpace(requestLine, 3));
            if (splittedRequestLine.size() != 3) {
                throw Error_t("Bad HTTP request: %s",
                    requestLine.substr(0, 30).c_str());
            }

            if (splittedRequestLine[2] != "HTTP/1.0" && splittedRequestLine[2] != "HTTP/1.1") {
                throw Error_t(
                    "Bad HTTP protocol version or type: %s",
                    splittedRequestLine[2].c_str());
            }

            request.method = splittedRequestLine[0];
            request.unparsedUri = splittedRequestLine[1];
            size_t pos(request.unparsedUri.find("?"));
            size_t pos2(request.unparsedUri.find("#"));
            if (pos == std::string::npos) {
                if (pos2 == std::string::npos) {
                    request.uri = request.unparsedUri;
                } else {
                    request.uri = request.unparsedUri.substr(0, pos2);
                }
            } else {
                if (pos2 == std::string::npos) {
                    request.uri = request.unparsedUri.substr(0, pos);
                } else {
                    request.uri = request.unparsedUri.substr(0, std::min(pos, pos2));
                }
            }
            request.protocol = splittedRequestLine[2];

            io.readHeader(request.headers);
            request.contentType = "text/plain";
            request.headers.get("Content-Type", request.contentType);

            Reader_t reader(request.data);
            FRPC::DataSink_t dataSink(reader);
            io.readContent(request.headers, dataSink, true);
        } catch (...) {
            LOG(WARN2, "Bad request: %s", requestLine.c_str());
            std::string data(
                "HTTP/1.0 400 Bad Request\r\n"
                "Server: ThreadServer/CppHttpHandler Linux\r\n\r\n");
            try {
                io.sendData(data.c_str(), data.size(), false);
            } catch (const std::exception &e) {
                throw Error_t("Can't send data: %s", e.what());
            }
            handler->work.release();
            return;
        }

        Response_t response(request);
        response.contentType = "text/plain";

        std::list<std::pair<boost::regex, Method_t*> >::const_iterator imethodRegistry;
        for (imethodRegistry = handler->methodRegistry->begin() ;
             imethodRegistry != handler->methodRegistry->end() ;
             ++imethodRegistry) {

            boost::cmatch matches;
            if (boost::regex_match(request.uri.c_str(), matches, imethodRegistry->first)) {
                for (size_t i(1) ; i < matches.size() ; ++i) {
                    request.matchGroups.push_back(std::string(matches[i].first, matches[i].second));
                }
                break;
            }
        }

        if (imethodRegistry != handler->methodRegistry->end()) {
            try {
                imethodRegistry->second->call(request, response);
                response.headers.set("Content-Type", response.contentType);
            } catch (const HttpError_t &e) {
                response.status = e.code();
                response.data = e.what();
            } catch (const std::exception &e) {
                if (response.debugLogInfo.empty()) {
                    LOG(ERR3, "Method %s thrown an exception: %s",
                        request.uri.c_str(), e.what());
                } else {
                    LOG(ERR3, "[%s] Method %s thrown an exception: %s",
                        response.debugLogInfo.c_str(), request.uri.c_str(), e.what());
                }
                response.status = 500;
                response.data = e.what();
            } catch (...) {
                if (response.debugLogInfo.empty()) {
                    LOG(ERR3, "Method %s thrown an unknown exception",
                        request.uri.c_str());
                } else {
                    LOG(ERR3, "[%s] Method %s thrown an unknown exception",
                        response.debugLogInfo.c_str(), request.uri.c_str());
                }
                response.status = 500;
                response.data = "Unknown exception";
            }
        } else {
            response.status = 404;
            response.data += "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>";
            response.data += "The requested URL " + request.unparsedUri + " was not found on this server.<hr />";
            response.data += "</body></html>";
        }

        std::stringstream output;
        output << response.protocol << " " << response.status << " ";
        switch (response.status) {
        case 100: output << "Continue"; break;
        case 101: output << "Switching Protocols"; break;
        case 200: output << "OK"; break;
        case 201: output << "Created"; break;
        case 202: output << "Accepted"; break;
        case 203: output << "Non-Authoritative Information"; break;
        case 204: output << "No Content"; break;
        case 205: output << "Reset Content"; break;
        case 206: output << "Partial Content"; break;
        case 300: output << "Multiple Choices"; break;
        case 301: output << "Moved Permanently"; break;
        case 302: output << "Found"; break;
        case 303: output << "See Other"; break;
        case 304: output << "Not Modified"; break;
        case 305: output << "Use Proxy"; break;
        case 307: output << "Temporary Redirect"; break;
        case 400: output << "Bad Request"; break;
        case 401: output << "Unauthorized"; break;
        case 402: output << "Payment Required"; break;
        case 403: output << "Forbidden"; break;
        case 404: output << "Not Found"; break;
        case 405: output << "Method Not Allowed"; break;
        case 406: output << "Not Acceptable"; break;
        case 407: output << "Proxy Authentication Required"; break;
        case 408: output << "Request Timeout"; break;
        case 409: output << "Conflict"; break;
        case 410: output << "Gone"; break;
        case 411: output << "Length Required"; break;
        case 412: output << "Precondition Failed"; break;
        case 413: output << "Request Entity Too Large"; break;
        case 414: output << "Request-URI Too Long"; break;
        case 415: output << "Unsupported Media Type"; break;
        case 416: output << "Request Range Not Satisfiable"; break;
        case 417: output << "Expectation Failed"; break;
        case 500: output << "Internal Server Error"; break;
        case 501: output << "Not Implemented"; break;
        case 502: output << "Bad Gateway"; break;
        case 503: output << "Service Unavailable"; break;
        case 504: output << "Gateway Timeout"; break;
        case 505: output << "HTTP Version Not Supported"; break;
        default:
            if (!response.statusMessage.empty()) {
                output << response.statusMessage;
            } else {
                output << "Unknown";
            }
        }
        output << "\r\n" << response.headers << "\r\n";
        std::string outputStr(output.str());

        io.sendData(outputStr);
        io.sendData(response.data);

        if (!response.dontLog) {
            if (response.status / 100 < 4) {
                if (response.debugLogInfo.empty()) {
                    LOG(INFO2, "%d %s %s",
                        response.status, request.method.c_str(), request.unparsedUri.c_str());
                } else {
                    LOG(INFO2, "[%s] %d %s %s", response.debugLogInfo.c_str(),
                        response.status, request.method.c_str(), request.unparsedUri.c_str());
                }
            } else if (response.status / 100 < 5) {
                if (response.debugLogInfo.empty()) {
                    LOG(WARN2, "%d %s %s",
                        response.status, request.method.c_str(), request.unparsedUri.c_str());
                } else {
                    LOG(WARN2, "[%s] %d %s %s", response.debugLogInfo.c_str(),
                        response.status, request.method.c_str(), request.unparsedUri.c_str());
                }
            } else {
                if (response.debugLogInfo.empty()) {
                    LOG(ERR2, "%d %s %s",
                        response.status, request.method.c_str(), request.unparsedUri.c_str());
                } else {
                    LOG(ERR2, "[%s] %d %s %s", response.debugLogInfo.c_str(),
                        response.status, request.method.c_str(), request.unparsedUri.c_str());
                }
            }
        }
    } catch (const std::exception &e) {
        LOG(ERR2, "Exception: %s", e.what());
        handler->work.release();
        throw e;
    } catch (...) {
        LOG(ERR2, "Unknown exception");
        handler->work.release();
        throw;
    }
    handler->work.release();
}

CppHttpHandler_t::DlHandleGuard_t::DlHandleGuard_t(void *handle)
  : handle(handle)
{
}

CppHttpHandler_t::DlHandleGuard_t::~DlHandleGuard_t()
{
    if (handle) {
        dlclose(handle);
        handle = 0;
    }
}

void CppHttpHandler_t::DlHandleGuard_t::reset(void *_handle)
{
    if (handle) {
        dlclose(handle);
    }
    handle = _handle;
}

void* CppHttpHandler_t::DlHandleGuard_t::get()
{
    return handle;
}

#define HTTPERROR_MESSAGEBUFFERSIZE 65536

CppHttpHandler_t::HttpError_t::HttpError_t(const int _code,
                                           const char *format, ...)
  : Error_t(({
        va_list valist;
        char messageBuffer[HTTPERROR_MESSAGEBUFFERSIZE];
        va_start(valist, format);
        vsnprintf(messageBuffer, HTTPERROR_MESSAGEBUFFERSIZE, format, valist);
        va_end(valist);
        std::string result(messageBuffer);
        result;
    }).c_str()),
    _code(_code)
{
}

CppHttpHandler_t::HttpError_t::HttpError_t(const int _code,
                                           const std::string &message)
  : Error_t(message),
    _code(_code)
{
}

int CppHttpHandler_t::HttpError_t::code() const
{
    return _code;
}

#undef HTTPERROR_MESSAGEBUFFERSIZE

CppHttpHandler_t::Message_t::Message_t(const std::string &protocol)
  : status(200),
    protocol(protocol),
    headers(),
    data()
{
}

CppHttpHandler_t::Request_t::Request_t()
  : Message_t()
{
}

CppHttpHandler_t::Response_t::Response_t(const CppHttpHandler_t::Request_t &request)
  : Message_t(request.protocol),
    debugLogInfo(),
    dontLog(false)
{
}

void CppHttpHandler_t::loadModule(const std::string &filename,
                                  const std::string &symbol)
{
    moduleHandle.reset(dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL));
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

void CppHttpHandler_t::forbidden(boost::shared_ptr<SocketWork_t> socket)
{
    HttpIo_t io(
        socket->getSocket()->native(),
        readTimeout,
        writeTimeout,
        maxLineSize,
        maxRequestSize);

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
            "Server: ThreadServer/CppHttpHandler Linux\r\n\r\n");
        data = request[2] + data;
        io.sendData(data.c_str(), data.size(), false);
    } catch (...) {
        std::string data(
            "HTTP/1.0 400 Bad Request\r\n"
            "Server: ThreadServer/CppHttpHandler Linux\r\n\r\n");
        try {
            io.sendData(data.c_str(), data.size(), false);
        } catch (const std::exception &e) {
            throw Error_t("Can't send data: %s", e.what());
        }
        throw;
    }
}

CppHttpHandler_t::Method_t::Method_t()
{
}

CppHttpHandler_t::Method_t::~Method_t()
{
}

void CppHttpHandler_t::registerMethod(const std::string &location,
                                      CppHttpHandler_t::Method_t *method)
{
    methodRegistry->push_back(
        std::make_pair(
            boost::regex(location),
            method));
}

CppHttpHandler_t::Parameters_t::Parameters_t()
  : data()
{
}

void CppHttpHandler_t::Parameters_t::parse(const std::string &params)
{
    boost::char_separator<char> sep("&");
    boost::tokenizer<boost::char_separator<char> > tokens(params, sep);
    BOOST_FOREACH(std::string t, tokens) {
        size_t pos(t.find("="));
        if (pos == std::string::npos) {
            data[unescape(t)].push_back("");
        } else {
            data[unescape(t.substr(0, pos))].push_back(unescape(t.substr(pos+1)));
        }
    }
}

int CppHttpHandler_t::Parameters_t::unhex(const char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 0xa;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 0xa;
    }
    throw std::runtime_error("invalid hex digit");
}

std::string CppHttpHandler_t::Parameters_t::unescape(const std::string &s)
{
    std::string result;
    for (size_t i(0) ; i < s.size() ; ++i) {
        if (s[i] == '+') {
            result.push_back(' ');
        } else if (s[i] == '%' && i < s.size() - 2) {
            int d1, d2;
            try {
                d1 = unhex(s[i+1]);
                d2 = unhex(s[i+2]);
                result.push_back(char(d1*16+d2));
                i += 2;
            } catch (const std::exception &e) {
                result.push_back(s[i]);
            }
        } else {
            result.push_back(s[i]);
        }
    }
    return result;
}

bool CppHttpHandler_t::Parameters_t::getFirstBool(const std::string &name) const
{
    boost::optional<std::string> data(getFirst<std::string>(name));
    return data && (*data == "1" || *data == "true" || *data == "on");
}

std::list<bool> CppHttpHandler_t::Parameters_t::getBool(const std::string &name) const
{
    std::list<std::string> data(get<std::string>(name));
    std::list<bool> result;
    std::transform(data.begin(), data.end(), std::back_inserter(result),
        boost::lambda::_1 == "1" || boost::lambda::_1 == "true" || boost::lambda::_1 == "on");
    return result;
}

CppHttpHandler_t::MimeParameters_t::File_t::File_t()
  : data(),
    contentType(),
    filename()
{
}

CppHttpHandler_t::MimeParameters_t::MimeParameters_t()
  : CppHttpHandler_t::Parameters_t(),
    fileData()
{
}

void CppHttpHandler_t::MimeParameters_t::parseMime(const std::string &params)
{
    mimetic::NullCodec nullDecoder;
    mimetic::Base64::Decoder base64Decoder;
    mimetic::QP::Decoder qpDecoder;

    mimetic::MimeEntity mimeMessage(params.begin(), params.end());
    mimetic::MimeEntityList &parts(mimeMessage.body().parts());
    for (mimetic::MimeEntityList::iterator iparts(parts.begin()) ;
         iparts != parts.end() ;
         ++iparts) {

        File_t file;
        std::string name;

        mimetic::MimeEntity &part(**iparts);

        mimetic::Header &header(part.header());

        mimetic::ContentType &contentType(header.contentType());
        file.contentType = contentType.type() + "/" + contentType.subtype();

        mimetic::ContentDisposition &contentDisposition(header.contentDisposition());
        mimetic::ContentDisposition::ParamList &params(contentDisposition.paramList());
        for (mimetic::ContentDisposition::ParamList::const_iterator iparams(params.begin()) ;
             iparams != params.end() ;
             ++iparams) {

            const mimetic::istring &key(iparams->name());
            if (key == "name") {
                name = iparams->value();
            } else if (key == "filename") {
                file.filename = iparams->value();
            }
        }

        mimetic::Body &body(part.body());
        if (header.contentTransferEncoding().mechanism() == base64Decoder.name()) {
            mimetic::code(body.begin(), body.end(), base64Decoder, std::back_inserter(file.data));
        } else if (header.contentTransferEncoding().mechanism() == qpDecoder.name()) {
            mimetic::code(body.begin(), body.end(), qpDecoder, std::back_inserter(file.data));
        } else {
            mimetic::code(body.begin(), body.end(), nullDecoder, std::back_inserter(file.data));
        }

        if (file.filename.empty() && file.contentType == "/") {
            data[name].push_back(file.data);
        } else {
            fileData[name].push_back(file);
        }
    }
}

const std::vector<CppHttpHandler_t::MimeParameters_t::File_t>& CppHttpHandler_t::MimeParameters_t::getFiles(const std::string &name) const
{
    FileData_t::const_iterator ifileData(fileData.find(name));
    if (ifileData == fileData.end()) {
        return empty;
    } else {
        return ifileData->second;
    }
}

const std::map<std::vector<size_t>, const CppHttpHandler_t::MimeParameters_t::File_t&>
CppHttpHandler_t::MimeParameters_t::getIndexedFiles(const std::string &name) const
{
    std::map<std::vector<size_t>, const File_t&> result;
    boost::regex regex(boost::algorithm::replace_all_copy(name, "[]", "\\[([0-9]+)\\]"));
    for (FileData_t::const_iterator ifileData(fileData.begin()) ;
         ifileData != fileData.end() ;
         ++ifileData) {

        std::vector<size_t> indexes;
        boost::cmatch matches;
        if (!boost::regex_match(ifileData->first.c_str(), matches, regex)) {
            continue;
        }

        for (size_t i(1) ; i < matches.size() ; ++i) {
            indexes.push_back(boost::lexical_cast<size_t>(std::string(matches[i].first, matches[i].second)));
        }

        result.insert(std::pair<std::vector<size_t>, const File_t&>(indexes, ifileData->second.front()));
    }

    return result;
}

SocketWork_t* CppHttpHandler_t::getWork()
{
    return work.get();
}

CppHttpHandler_t::Module_t::Module_t(CppHttpHandler_t *handler)
  : handler(handler)
{
}

CppHttpHandler_t::Module_t::~Module_t()
{
}

void CppHttpHandler_t::Module_t::threadCreate()
{
}

void CppHttpHandler_t::Module_t::threadDestroy()
{
}

} // namespace ThreadServer

extern "C" {
    ThreadServer::CppHttpHandler_t* cpphttphandler(ThreadServer::ThreadServer_t *threadServer,
                                                   const std::string &name,
                                                   const size_t workerCount)
    {
        return new ThreadServer::CppHttpHandler_t(threadServer, name, workerCount);
    }
}



#include <dbglog.h>

#include <threadserver/handlers/dummyhandler/dummyhandler.h>

namespace ThreadServer {

DummyHandler_t::DummyHandler_t(ThreadServer_t *threadServer,
                               const std::string &name,
                               const size_t workerCount)
 : Handler_t(threadServer, name, workerCount)
{
    LOG(INFO4, "Creating handler");
}

DummyHandler_t::~DummyHandler_t()
{
    LOG(INFO4, "Destroying workers");
    destroyWorkers();
    LOG(INFO4, "Destroying handler");
}

Handler_t::Worker_t* DummyHandler_t::createWorker(Handler_t *handler)
{
    return new Worker_t(dynamic_cast<DummyHandler_t*>(handler));
}

DummyHandler_t::Worker_t::Worker_t(DummyHandler_t *handler)
  : Handler_t::Worker_t(handler)
{
    LOG(INFO4, "Creating worker");
}

DummyHandler_t::Worker_t::~Worker_t()
{
    LOG(INFO4, "Destroying worker");
}

void DummyHandler_t::Worker_t::handle(boost::shared_ptr<SocketWork_t> socket)
{
    boost::asio::streambuf request;
    boost::asio::read_until(*socket->getSocket(), request, "\r\n\r\n");

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 10000000;
    nanosleep(&ts, 0);

    std::string s("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nHello World!\r\n");
    boost::asio::write(*socket->getSocket(), boost::asio::buffer(s.data(), s.size()));
}

} // namespace ThreadServer

extern "C" {
    ThreadServer::DummyHandler_t* dummyhandler(ThreadServer::ThreadServer_t *threadServer,
                                               const std::string &name,
                                               const size_t workerCount)
    {
        return new ThreadServer::DummyHandler_t(threadServer, name, workerCount);
    }
}

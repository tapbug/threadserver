
#ifndef THREADSERVER_LISTENER_H
#define THREADSERVER_LISTENER_H

#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

#include <threadserver/handler.h>
#include <threadserver/network.h>
#include <threadserver/work.h>

namespace ThreadServer {

class Listener_t : public boost::noncopyable {
friend class ThreadServer_t;
public:
    Listener_t(const std::string &address,
               const std::string &handlerName,
               const bool &allowFirst,
               const std::vector<Network_t> &allowedNetworks,
               const std::vector<Network_t> &deniedNetworks);

    std::string getAddress() const;

    std::string getHandlerName() const;

    void run(boost::asio::io_service &ioService);

    void stop();

private:
    void parseAddress();

    void setHandler(Handler_t *_handler);

    void listen(boost::asio::io_service *ioService);

    void asyncAccept(boost::asio::io_service *ioService);

    void accept(boost::shared_ptr<SocketWork_t> socket,
                boost::asio::io_service *ioService,
                const boost::system::error_code &ec);

    void isForbidden(bool &result,
                     const bool value,
                     const boost::asio::ip::address_v4 &address,
                     const std::vector<Network_t> &networks) const;

    bool isForbidden(boost::shared_ptr<SocketWork_t> socket) const;

    const std::string address;
    std::string ip;
    uint16_t port;
    const std::string handlerName;
    bool allowFirst;
    std::vector<Network_t> allowedNetworks;
    std::vector<Network_t> deniedNetworks;
    Handler_t *handler;
    std::auto_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    std::auto_ptr<boost::thread> acceptorThread;
};

} // namespace ThreadServer

#endif // THREADSERVER_LISTENER_H

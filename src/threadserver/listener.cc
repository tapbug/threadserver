
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <dbglog.h>

#include <threadserver/error.h>
#include <threadserver/listener.h>

namespace ThreadServer {

Listener_t::Listener_t(const std::string &address,
                       const std::string &handlerName,
                       const bool &allowFirst,
                       const std::vector<Network_t> &allowedNetworks,
                       const std::vector<Network_t> &deniedNetworks)
  : boost::noncopyable(),
    address(address),
    ip(),
    port(0),
    handlerName(handlerName),
    allowFirst(true),
    allowedNetworks(allowedNetworks),
    deniedNetworks(deniedNetworks),
    handler(0),
    acceptor(0)
{
    parseAddress();

    // test listen
    {
        boost::asio::io_service ioService;
        boost::asio::ip::tcp::acceptor testAcceptor(ioService);
        boost::asio::ip::tcp::endpoint endpoint;
        if (!ip.empty()) {
            endpoint.address(boost::asio::ip::address::from_string(ip));
        }
        endpoint.port(port);
        testAcceptor.open(endpoint.protocol());
        testAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        testAcceptor.bind(endpoint);
        testAcceptor.close();
    }
}

void Listener_t::parseAddress()
{
    size_t pos(address.rfind(":"));
    if (pos == std::string::npos) {
        throw Error_t("Invalid listen address %s", address.c_str());
    }

    ip = address.substr(0, pos);
    port = boost::lexical_cast<uint16_t>(address.substr(pos+1));

    if (!port) {
        throw Error_t("Invalid listen address %s", address.c_str());
    }

    if (ip == "*") {
        ip = "";
    }
}

std::string Listener_t::getAddress() const
{
    return address;
}

std::string Listener_t::getHandlerName() const
{
    return handlerName;
}

void Listener_t::setHandler(Handler_t *_handler)
{
    handler = _handler;
}

void Listener_t::run(boost::asio::io_service &ioService)
{
    acceptor.reset(new boost::asio::ip::tcp::acceptor(ioService));

    boost::asio::ip::tcp::endpoint endpoint;

    if (!ip.empty()) {
        endpoint.address(boost::asio::ip::address::from_string(ip));
    }
    endpoint.port(port);

    acceptor->open(endpoint.protocol());
    acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor->bind(endpoint);
    acceptor->listen();

    acceptorThread.reset(
        new boost::thread(boost::bind(&Listener_t::listen, this, &ioService)));
}

void Listener_t::stop()
{
    acceptor->cancel();
    acceptor->close();
    acceptorThread->join();
    acceptorThread.reset(0);
}

void Listener_t::listen(boost::asio::io_service *ioService)
{
    logAppName((getAddress() + "["
        + boost::lexical_cast<std::string>(getpid())
        + ":"
        + boost::lexical_cast<std::string>(pthread_self()) + "]").c_str());

    asyncAccept(ioService);
}

void Listener_t::asyncAccept(boost::asio::io_service *ioService)
{
    boost::shared_ptr<SocketWork_t> socket(
        new SocketWork_t(this, boost::shared_ptr<boost::asio::ip::tcp::socket>(
            new boost::asio::ip::tcp::socket(*ioService))));

    acceptor->async_accept(*socket->getSocket(), boost::bind(
        &Listener_t::accept, this, socket, ioService,
        boost::asio::placeholders::error));
}

void Listener_t::accept(boost::shared_ptr<SocketWork_t> socket,
                        boost::asio::io_service *ioService,
                        const boost::system::error_code &error)
{
    if (!error) {
        asyncAccept(ioService);
        socket->forbidden = isForbidden(socket);
        handler->enque(socket);
    } else if (error != boost::system::posix_error::operation_canceled) {
        LOG(ERR3, "Listener on %s can't accept connection: %s",
            address.c_str(), boost::system::system_error(error).what());
    }
}

void Listener_t::isForbidden(bool &result,
                             const bool value,
                             const boost::asio::ip::address_v4 &address,
                             const std::vector<Network_t> &networks) const
{
    for (std::vector<Network_t>::const_iterator inetworks(networks.begin()) ;
         inetworks != networks.end() ;
         ++inetworks) {

        if (inetworks->contains(address)) {
            result = value;
            return;
        }
    }
}

bool Listener_t::isForbidden(boost::shared_ptr<SocketWork_t> socket) const
{
    bool result(true);

    if (!socket->getSocket()->remote_endpoint().address().is_v4()) {
        return true;
    }

    boost::asio::ip::address_v4 address(socket->getSocket()->remote_endpoint().address().to_v4());

    if (allowFirst) {
        isForbidden(result, false, address, allowedNetworks);
        isForbidden(result, true, address, deniedNetworks);
    } else {
        isForbidden(result, true, address, deniedNetworks);
        isForbidden(result, false, address, allowedNetworks);
    }

    return result;
}

} // namespace ThreadServer

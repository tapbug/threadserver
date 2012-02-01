
#include <threadserver/listener.h>
#include <threadserver/work.h>

namespace ThreadServer {

Work_t::Work_t()
  : boost::noncopyable()
{
}

SocketWork_t::SocketWork_t(const Listener_t *listener,
                           boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
  : Work_t(),
    listener(listener),
    socket(socket),
    forbidden(false)
{
}

const Listener_t* SocketWork_t::getListener() const
{
    return listener;
}

boost::shared_ptr<boost::asio::ip::tcp::socket> SocketWork_t::getSocket()
{
    return socket;
}

std::string SocketWork_t::getClientAddress() const
{
    return socket->remote_endpoint().address().to_string();
}

} // namespace ThreadServer


#ifndef THREADSERVER_WORK_H
#define THREADSERVER_WORK_H

#include <queue>
#include <string>
#include <boost/asio.hpp>
#include <boost/utility.hpp>

namespace ThreadServer {

class Listener_t;

class Work_t : public boost::noncopyable {
public:
    Work_t();
};

class SocketWork_t : public Work_t {
public:
    SocketWork_t(const Listener_t *listener,
                 boost::shared_ptr<boost::asio::ip::tcp::socket> socket);

    const Listener_t* getListener() const;

    boost::shared_ptr<boost::asio::ip::tcp::socket> getSocket();

    std::string getClientAddress() const;

private:
    const Listener_t *listener;
    boost::shared_ptr<boost::asio::ip::tcp::socket> socket;

public:
    bool forbidden;
};

} // namespace ThreadServer

#endif // THREADSERVER_WORK_H


#ifndef THREADSERVER_NETWORK_H
#define THREADSERVER_NETWORK_H

#include <boost/asio.hpp>

namespace ThreadServer {

class Network_t {
public:
    Network_t(const boost::asio::ip::address_v4 &address,
              const boost::asio::ip::address_v4 &netmask);

    static Network_t parse(const std::string &addressWithNetmask);

    static std::vector<Network_t> parse(const std::vector<std::string> &addressesWithNetmasks);

    bool contains(const std::string &_address) const;

    bool contains(const boost::asio::ip::address_v4 &_address) const;

    boost::asio::ip::address_v4 getAddress() const;

    boost::asio::ip::address_v4 getNetmask() const;

private:
    boost::asio::ip::address_v4 address;
    boost::asio::ip::address_v4 netmask;
};

} // namespace ThreadServer

#endif // THREADSERVER_NETWORK_H

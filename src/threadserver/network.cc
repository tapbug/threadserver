
#include <boost/lexical_cast.hpp>
#include <threadserver/network.h>

namespace ThreadServer {

Network_t::Network_t(const boost::asio::ip::address_v4 &address,
                     const boost::asio::ip::address_v4 &netmask)
  : address(address),
    netmask(netmask)
{
}

Network_t Network_t::parse(const std::string &addressWithNetmask)
{
    std::string address(addressWithNetmask);
    std::string netmask("255.255.255.255");
    size_t pos(address.find("/"));
    if (pos != std::string::npos) {
        netmask = address.substr(pos+1);
        address = address.substr(0, pos);
        if (netmask.size() <= 2) {
            uint64_t netmaskAsInt(0x100000000);
            netmaskAsInt -= (0x100000000 >> boost::lexical_cast<int>(netmask));
            netmask = boost::lexical_cast<std::string>(netmaskAsInt >> 24) + "."
                + boost::lexical_cast<std::string>((netmaskAsInt >> 16) & 0xff) + "."
                + boost::lexical_cast<std::string>((netmaskAsInt >> 8) & 0xff) + "."
                + boost::lexical_cast<std::string>(netmaskAsInt & 0xff);
        }
    }

    boost::asio::ip::address_v4 address_v4(boost::asio::ip::address_v4::from_string(address));
    boost::asio::ip::address_v4 netmask_v4(boost::asio::ip::address_v4::from_string(netmask));

    return Network_t(
        boost::asio::ip::address_v4(address_v4.to_ulong() & netmask_v4.to_ulong()),
        netmask_v4);
}

std::vector<Network_t> Network_t::parse(const std::vector<std::string> &addressesWithNetmasks)
{
    std::vector<Network_t> result;
    for (std::vector<std::string>::const_iterator iaddressesWithNetmasks(addressesWithNetmasks.begin()) ;
         iaddressesWithNetmasks != addressesWithNetmasks.end() ;
         ++iaddressesWithNetmasks) {

        result.push_back(parse(*iaddressesWithNetmasks));
    }
    return result;
}

bool Network_t::contains(const std::string &_address) const
{
    return (boost::asio::ip::address_v4::from_string(_address).to_ulong() & netmask.to_ulong()) == address.to_ulong();
}

bool Network_t::contains(const boost::asio::ip::address_v4 &_address) const
{
    return (_address.to_ulong() & netmask.to_ulong()) == address.to_ulong();
}

boost::asio::ip::address_v4 Network_t::getAddress() const
{
    return address;
}

boost::asio::ip::address_v4 Network_t::getNetmask() const
{
    return netmask;
}

} // namespace ThreadServer

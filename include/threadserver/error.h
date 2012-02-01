
#ifndef THREADSERVER_ERROR_H
#define THREADSERVER_ERROR_H

#include <stdexcept>
#include <string>

namespace ThreadServer {

class Error_t : public std::runtime_error {
public:
    Error_t(const char *format, ...);

    Error_t(const std::string &message);
};

} // namespace ThreadServer

#endif // THREADSERVER_ERROR_H

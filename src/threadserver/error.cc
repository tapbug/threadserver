
#include <stdarg.h>
#include <stdio.h>

#include <threadserver/error.h>

#define THREADSERVER_ERROR_MESSAGEBUFFERSIZE 65536

namespace ThreadServer {

Error_t::Error_t(const char *format, ...)
  : std::runtime_error(({
        va_list valist;
        char messageBuffer[THREADSERVER_ERROR_MESSAGEBUFFERSIZE];
        va_start(valist, format);
        vsnprintf(messageBuffer, THREADSERVER_ERROR_MESSAGEBUFFERSIZE, format, valist);
        va_end(valist);
        std::string result(messageBuffer);
        result;
    }).c_str())
{
}

Error_t::Error_t(const std::string &message)
  : std::runtime_error(message.c_str())
{
}

#undef THREADSERVER_ERROR_MESSAGEBUFFERSIZE

} // namespace ThreadServer

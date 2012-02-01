
#include <dbglog.h>
#include <signal.h>
#include <boost/bind.hpp>

#include <threadserver/threadserver.h>

bool run = true;

void signalHandler(int signo)
{
    if (ThreadServer::childPid != 0) {
        kill(ThreadServer::childPid, signo);
    }

    if (signo == SIGINT || signo == SIGTERM) {
        LOG(INFO4, "Shutting down (%s)", sys_siglist[signo]);
        run = false;
    } else if (signo == SIGHUP || signo == SIGUSR1) {
        LOG(INFO4, "Reopening log (%s)", sys_siglist[signo]);
        logReopen();
        LOG(INFO4, "Reopened log");
    }
}

int main(int argc, char **argv)
{
    ThreadServer::childPid = 0;

    logStderr(1);

    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGUSR1, signalHandler);

    try {
        ThreadServer::ThreadServer_t server(argc, argv);

        server.run();

        for (;;) {
            if (!run) {
                break;
            }
            sleep(1);
        }

        server.stop();
    } catch (const std::exception &e) {
        LOG(FATAL4, "Can't initialise server: %s", e.what());
        return 1;
    }

    LOG(INFO4, "Server shutdown complete");

    return 0;
}

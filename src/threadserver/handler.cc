
#include <boost/lexical_cast.hpp>
#include <dbglog.h>

#include <threadserver/handler.h>

namespace ThreadServer {

Handler_t::Handler_t(ThreadServer_t *threadServer,
                     const std::string &name,
                     const size_t workerCount)
  : boost::noncopyable(),
    threadServer(threadServer),
    name(name),
    workerCount(workerCount),
    workerPool(),
    mutex(),
    waitingCond(),
    notWaitingCond(),
    waiting(true),
    exiting(false)
{
}

Handler_t::~Handler_t()
{
}

void Handler_t::createWorkers()
{
    for (size_t i(workerCount) ; i ; --i) {
        workerPool.push_back(new boost::thread(boost::bind(&Handler_t::run, this)));
    }
}

void Handler_t::destroyWorkers()
{
    {
        boost::mutex::scoped_lock lock(mutex);
        exiting = true;
        waitingCond.notify_all();
    }

    for (WorkerPool_t::iterator iworkerPool(workerPool.begin()) ;
         iworkerPool != workerPool.end() ;
         ++iworkerPool) {

        (*iworkerPool)->join();
        delete *iworkerPool;
    }

    workerPool.clear();
}

std::string Handler_t::getName() const
{
    return name;
}

void Handler_t::enque(boost::shared_ptr<SocketWork_t> socket)
{
    boost::mutex::scoped_lock lock(mutex);
    while (!waiting) {
        notWaitingCond.wait(lock);
    }
    workQueue.push(socket);
    waiting = false;
    waitingCond.notify_one();
}

void Handler_t::run()
{
    logAppName((getName() + "["
        + boost::lexical_cast<std::string>(getpid())
        + ":"
        + boost::lexical_cast<std::string>(pthread_self()) + "]").c_str());

    std::auto_ptr<Worker_t> worker(this->createWorker(this));
    worker->run();
}

Handler_t::Worker_t::Worker_t(Handler_t *handler)
  : handler(handler)
{
}

Handler_t::Worker_t::~Worker_t()
{
}

void Handler_t::Worker_t::run()
{
    for (;;) {
        boost::shared_ptr<SocketWork_t> socket;
        {
            boost::mutex::scoped_lock lock(handler->mutex);
            while (handler->waiting && !handler->exiting) {
                handler->waitingCond.wait(lock);
            }
            if (!handler->exiting) {
                socket = handler->workQueue.front();
                handler->workQueue.pop();
                handler->waiting = true;
                handler->notWaitingCond.notify_one();
            }
        }

        if (handler->exiting) {
            break;
        }

        try {
            handle(socket);
        } catch (const boost::system::system_error &e) {
            if (e.code() == boost::system::posix_error::broken_pipe) {
                LOG(ERR2, "Handler %s: Connection closed by foreign host",
                    handler->name.c_str());
            } else {
                LOG(ERR3, "Handler %s thrown an exception: %s",
                    handler->name.c_str(), e.what());
            }
        } catch (const std::exception &e) {
            LOG(ERR3, "Handler %s thrown an exception: %s",
                handler->name.c_str(), e.what());
        } catch (...) {
            LOG(ERR3, "Handler %s thrown an unknown exception",
                handler->name.c_str());
        }
    }
}

} // namespace ThreadServer

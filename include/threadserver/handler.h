
#ifndef THREADSERVER_HANDLER_H
#define THREADSERVER_HANDLER_H

#include <queue>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <threading/threading.h>

#include <threadserver/work.h>

namespace ThreadServer {

class ThreadServer_t;

class Handler_t : public boost::noncopyable {
friend class ThreadServer_t;
public:
    class Worker_t {
    public:
        Worker_t(Handler_t *handler);

        virtual ~Worker_t();

        virtual void handle(boost::shared_ptr<SocketWork_t> socket) = 0;

        virtual void run();

    protected:
        Handler_t *handler;
    };

    Handler_t(ThreadServer_t *threadServer,
              const std::string &name,
              const size_t workerCount);

    virtual ~Handler_t();

    std::string getName() const;

    virtual Worker_t* createWorker(Handler_t *handler) = 0;

    void enqueue(boost::shared_ptr<SocketWork_t> socket);

protected:
    void destroyWorkers();

private:
    typedef std::vector<boost::thread*> WorkerPool_t;

    virtual void createWorkers();

    virtual void run();

public:
    ThreadServer_t *threadServer;
    const std::string name;

private:
    size_t workerCount;
    WorkerPool_t workerPool;
    threading::queue<boost::shared_ptr<SocketWork_t> > workQueue;
};

} // namespace ThreadServer

#endif // THREADSERVER_HANDLER_H

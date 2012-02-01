
#ifndef JOB_H
#define JOB_H

#include <boost/thread.hpp>

namespace ThreadServer {

class Job_t {
public:
    Job_t(const std::string &name,
          const size_t workers,
          const time_t sleep = 1);

    virtual ~Job_t();
    virtual void work() = 0;
    virtual void threadCreate();
    virtual void threadDestroy();
    void start();
    void stop();

private:
    virtual void worker();

protected:
    const std::string name;

private:
    const time_t sleep;
    std::list<boost::shared_ptr<boost::thread> > threads;
};

} // namespace ThreadServer

#endif // JOB_H


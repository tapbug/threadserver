
#include <dbglog.h>
#include <boost/lexical_cast.hpp>
#include <threadserver/job.h>

namespace ThreadServer {

Job_t::Job_t(const std::string &name,
             const size_t workers,
             const time_t sleep)
  : name(name),
    sleep(sleep)
{
    for (size_t i(0) ; i < workers ; ++i) {
        threads.push_back(boost::shared_ptr<boost::thread>());
    }
}

Job_t::~Job_t()
{
}

void Job_t::stop()
{
    LOG(INFO3, "Requesting job %s to finish", name.c_str());
    for (std::list<boost::shared_ptr<boost::thread> >::iterator ithreads(threads.begin()) ;
         ithreads != threads.end() ;
         ++ithreads) {

        (*ithreads)->interrupt();
        (*ithreads)->join();
        ithreads->reset();
    }
    LOG(INFO3, "Job %s finished", name.c_str());
}

void Job_t::start()
{
    LOG(INFO3, "Starting job %s", name.c_str());
    for (std::list<boost::shared_ptr<boost::thread> >::iterator ithreads(threads.begin()) ;
         ithreads != threads.end() ;
         ++ithreads) {

        ithreads->reset(new boost::thread(boost::bind(&Job_t::worker, this)));
    }
    LOG(INFO3, "Job %s started", name.c_str());
}

void Job_t::threadCreate()
{
}

void Job_t::threadDestroy()
{
}

void Job_t::worker()
{
    logAppName((name + "["
        + boost::lexical_cast<std::string>(getpid())
        + ":"
        + boost::lexical_cast<std::string>(pthread_self()) + "]").c_str());

    LOG(INFO2, "Job worker started");

    threadCreate();

    LOG(INFO1, "Job worker entering main loop");
    for (;;) {
        try {
            boost::this_thread::sleep(boost::posix_time::seconds(sleep));
            work();
        } catch (const boost::thread_interrupted &e) {
            break;
        } catch (const std::exception &e) {
            LOG(ERR3, "Job worker thrown exception: %s", e.what());
        } catch (...) {
            LOG(ERR3, "Job worker thrown an unknown exception");
        }
    }

    threadDestroy();

    LOG(INFO2, "Job worker finished");
}

} // namespace ThreadServer


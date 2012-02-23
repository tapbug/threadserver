
AM_CXXFLAGS = -Werror -Wall -O2 -D_FILE_OFFSET_BITS=64 -fPIC -g -ggdb3 ${CXXEXTRAFLAGS} -I../../include

lib_LTLIBRARIES = libthreadserver.la

sbin_PROGRAMS = threadserver

libthreadserver_la_SOURCES = \
    configuration.cc \
    error.cc \
    handler.cc \
    job.cc \
    listener.cc \
    network.cc \
    threadserver.cc \
    work.cc

threadserver_SOURCES = \
    main.cc

threadserver_LDADD = \
    libthreadserver.la

library_includedir = $(includedir)/threadserver

library_include_HEADERS = \
    ../../include/threadserver/configuration.h \
    ../../include/threadserver/error.h \
    ../../include/threadserver/handler.h \
    ../../include/threadserver/job.h \
    ../../include/threadserver/listener.h \
    ../../include/threadserver/network.h \
    ../../include/threadserver/threadserver.h \
    ../../include/threadserver/work.h

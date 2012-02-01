
AM_CXXFLAGS = -Werror -Wall -O2 -D_FILE_OFFSET_BITS=64 -fPIC -g ${CXXEXTRAFLAGS} -I../../../include

handlerdir = $(prefix)/lib/threadserver/handlers

handler_LTLIBRARIES = dummyhandler.la

dummyhandler_la_LDFLAGS = -module -avoid-versions -L../../threadserver

dummyhandler_la_SOURCES = \
    dummyhandler.cc

dummyhandler_la_LIBADD = \
    -lthreadserver

libdir = $(prefix)/lib/threadserver

library_includedir = $(includedir)/threadserver/handlers/dummyhandler

library_include_HEADERS = \
    ../../../include/threadserver/handlers/dummyhandler/dummyhandler.h


AM_CXXFLAGS = -Werror -Wall -O2 -D_FILE_OFFSET_BITS=64 -fPIC -g ${CXXEXTRAFLAGS} -I../../../include

handlerdir = $(prefix)/lib/threadserver/handlers

handler_LTLIBRARIES = cppfrpchandler.la

cppfrpchandler_la_LDFLAGS = -module -avoid-versions -L../../threadserver

cppfrpchandler_la_SOURCES = \
    cppfrpchandler.cc

cppfrpchandler_la_LIBADD = \
    -lthreadserver

library_includedir = $(includedir)/threadserver/handlers/cppfrpchandler

library_include_HEADERS = \
    ../../../include/threadserver/handlers/cppfrpchandler/cppfrpchandler.h


AM_CXXFLAGS = -Werror -Wall -O2 -D_FILE_OFFSET_BITS=64 -fPIC -g ${CXXEXTRAFLAGS} -I../../../include

handlerdir = $(prefix)/lib/threadserver/handlers

handler_LTLIBRARIES = cpphttphandler.la

cpphttphandler_la_LDFLAGS = -module -avoid-versions -L../../threadserver

cpphttphandler_la_SOURCES = \
    cpphttphandler.cc \
    json.cc

cpphttphandler_la_LIBADD = \
    -lthreadserver

library_includedir = $(includedir)/threadserver/handlers/cpphttphandler

library_include_HEADERS = \
    ../../../include/threadserver/handlers/cpphttphandler/cpphttphandler.h \
    ../../../include/threadserver/handlers/cpphttphandler/json.h


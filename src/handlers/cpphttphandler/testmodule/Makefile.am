
AM_CXXFLAGS = -Werror -Wall -O2 -D_FILE_OFFSET_BITS=64 -fPIC -g ${CXXEXTRAFLAGS} -I../../../../include

moduledir = $(prefix)/lib/threadserver/handlers/modules

module_LTLIBRARIES = cpphttptestmodule.la

cpphttptestmodule_la_LDFLAGS = -module -avoid-versions -L../../../threadserver

cpphttptestmodule_la_SOURCES = \
    core.cc \
    httpinterface.cc \
    testmodule.cc

cpphttptestmodule_la_LIBADD = \
    -lthreadserver \
    ../cpphttphandler.la

install:



AM_CXXFLAGS = -Werror -Wall -O2 -D_FILE_OFFSET_BITS=64 -fPIC -g ${CXXEXTRAFLAGS} -I../../../../include

moduledir = $(prefix)/lib/threadserver/handlers/modules

module_LTLIBRARIES = cppfrpctestmodule.la

cppfrpctestmodule_la_LDFLAGS = -module -avoid-versions -L../../../threadserver

cppfrpctestmodule_la_SOURCES = \
    core.cc \
    frpcinterface.cc \
    testmodule.cc

cppfrpctestmodule_la_LIBADD = \
    -lthreadserver \
    ../cppfrpchandler.la

install:


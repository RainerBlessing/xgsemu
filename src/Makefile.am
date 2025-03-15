# these are the headers for your project
noinst_HEADERS = xgsemu.h xgsemuframe.h sdlpanel.h objfile.h

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = xgsemu

# the application source, library search path, and link libraries
xgsemu_SOURCES = xgsemu.cpp xgsemuframe.cpp cpu.cpp cpu.h filehandler.cpp \
	filehandler.h hex_file.cpp hex_file.h sxkey.h sdlpanel.cpp Video.cpp Video.h \
	objfile.cpp objfile.h findapppath.cpp
xgsemu_LDFLAGS = $(WX_LIBS) 


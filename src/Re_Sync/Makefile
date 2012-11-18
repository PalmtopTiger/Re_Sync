#############################################################################
# Makefile for building Re_Sync
#############################################################################

####### Compiler, tools and options

CC            = gcc
CXX           = g++
CFLAGS        = -O3 -Wall
CXXFLAGS      = -std=c++0x -O3 -Wall
INCPATH       = -I.
LINK          = g++
LFLAGS        = -Wl,-O1
LIBS          = -lboost_regex
DEL_FILE      = rm -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = ./

####### Files

SOURCES       = main.cpp format.cpp resync.cpp structure.cpp unix/io.cpp
OBJECTS       = main.o format.o resync.o structure.o io.o
DESTDIR       = bin
TARGET        = $(DESTDIR)/Re_Sync

first: all
####### Implicit rules

.SUFFIXES: .o .c .cpp

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o "$@" "$<"

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@$(CHK_DIR_EXISTS) $(DESTDIR) || $(MKDIR) $(DESTDIR)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

clean:
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) *~

####### Compile

io.o: unix/io.cpp
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o io.o unix/io.cpp


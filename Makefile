CXX=g++
RANLIB=ranlib

LIBSRC=VirtualMemory.cpp
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

VMLIB =  libVirtualMemory.a
TARGETS = $(VMLIB)

OBJS= VirtualMemory.o

TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
TARSRCS=VirtualMemory.cpp Makefile README


all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

%.o : %.cpp
		$(CXX) $(CXXFLAGS) -c $^

clean:
	$(RM) $(TARGETS) $(OBJS) $(VMLIB) $(LIBOBJ) $(TARNAME) *~ *core


tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
# Copyright (C) 2001  The Bochs Project
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA



.SUFFIXES: .cc

srcdir = .


SHELL = /bin/sh



CC = g++
CFLAGS = -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DBOCHS -pthread
CXX = g++
CXXFLAGS = -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -DBOCHS -pthread

LDFLAGS =
LIBS =  -ldl -lgtk-x11-2.0 -lgdk-x11-2.0 -lpangocairo-1.0 -latk-1.0 -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0 -lfontconfig -lfreetype
RANLIB = ranlib


# ===========================================================
# end of configurable options
# ===========================================================


BX_OBJS = instrument.o \
  arbiter.o \
  estimate.o \
  explore.o \
  kernel_specifics.o \
  lockset.o \
  messaging.o \
  pp.o \
  rand.o \
  rbtree.o \
  stack.o \
  student.o \
  test.o \
  user_specifics.o \
  user_sync.o \
  vector_clock.o \
  x86.o

BX_INCLUDES = instrument.h \
  arbiter.h \
  array_list.h \
  common.h \
  compiler.h \
  estimate.h \
  explore.h \
  found_a_bug.h \
  kernel_specifics.h \
  landslide.h \
  lockset.h \
  messaging.h \
  pp.h \
  rand.h \
  rbtree.h \
  save.h \
  schedule.h \
  simulator.h \
  stack.h \
  student_specifics.h \
  test.h \
  tree.h \
  user_specifics.h \
  user_sync.h \
  variable_queue.h \
  vector_clock.h \
  x86.h

BX_INCDIRS = -I../.. -I$(srcdir)/../.. -I. -I$(srcdir)/.

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(BX_INCDIRS) $< -o $@


.c.o:
	$(CC) -c $(CFLAGS) $(BX_INCDIRS) $< -o $@



libinstrument.a: $(BX_OBJS)
	rm -f  libinstrument.a
	ar rv $@ $(BX_OBJS)
	$(RANLIB) libinstrument.a

$(BX_OBJS): $(BX_INCLUDES)


clean:
	rm -f  *.o
	rm -f  *.a

dist-clean: clean
	rm -f  Makefile

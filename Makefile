CC = gcc
CXX = g++
LD = g++
CFLAGS = -I$(PWD)/include
CXXFLAGS = -I$(PWD)/include $(shell pkg-config --cflags protobuf) #-stdlib=libstdc++
LDFLAGS = -L$(PWD)/lib $(shell pkg-config --libs protobuf) -lprotoc #-stdlib=libstdc++

LIBS = $(PWD)/lib/sprotoc-c.a
#$(PWD)/lib/sprotoc.a
PWD = $(shell pwd)

# output executable can be any name, but must have
# relative path (so it lives in $(PWD)/$(SPROTOC)
SPROTOC = bin/sprotoc

export CC CXX LD CFLAGS CXXFLAGS LDFLAGS LIBS SPROTOC

all: $(SPROTOC)

tests: test/test.sh
	sh test/test.sh

distclean: clean
	rm -f $(SPROTOC)

clean:
	rm -f $(LIBS)
	make -C sprotoc clean
	make -C sprotoc-c clean

$(SPROTOC): $(LIBS)
	mkdir -p bin
	$(LD) -o $@ $^ $(LDFLAGS)

test/test.sh:	test $(LIBS) $(SPROTOC)
	make -C test gens
	make -C test test.sh

$(PWD)/lib/sprotoc.a:	sprotoc
	mkdir -p lib
	make -C sprotoc ../lib/sprotoc.a

$(PWD)/lib/sprotoc-c.a:	sprotoc-c
	mkdir -p lib
	make -C sprotoc-c ../lib/sprotoc-c.a

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $^

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $^


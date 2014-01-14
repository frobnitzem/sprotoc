CC = cc
CXX = c++
LD = c++
CFLAGS = -I$(PWD)/include
CXXFLAGS = -I$(PWD)/include $(shell pkg-config --cflags protobuf)
LDFLAGS = -L$(PWD)/lib $(shell pkg-config --libs protobuf) -lprotoc

LIBS = $(PWD)/lib/sprotoc-c.a $(PWD)/lib/sprotoc.a
PWD = $(shell pwd)

# output executable can be any name, but must have
# relative path (so it lives in $(PWD)/$(SPROTOC)
SPROTOC = bin/sprotoc

export CC LD CFLAGS CXXFLAGS LDFLAGS LIBS SPROTOC

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
	$(LD) $(LDFLAGS) -o $@ $^

test/test.sh:	test $(LIBS) $(SPROTOC)
	make -C test gens
	make -C test test.sh

$(PWD)/lib/sprotoc.a:	sprotoc
	make -C sprotoc ../lib/sprotoc.a

$(PWD)/lib/sprotoc-c.a:	sprotoc-c
	make -C sprotoc-c ../lib/sprotoc-c.a

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $^

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $^


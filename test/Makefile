CFLAGS += -ggdb -I../include
LIBS = ../lib/sprotoc.a

TESTS = test1 test_b

test.sh:	gens run_test.sh $(TESTS)
	@( echo "#!/bin/sh" >test.sh; \
	   echo "cd $(PWD)/test" >>test.sh; \
	   echo "TESTS=\"$(TESTS)\"" >>test.sh; \
	   cat run_test.sh >>test.sh; \
	)

test1:	test1.o test1.pb-c.o $(LIBS)
	$(CC) -o $@ $(LDFLAGS) $^

test_b:	test_b.o test_b.pb-c.o $(LIBS)
	$(CC) -o $@ $(LDFLAGS) $^


distclean:	clean

clean:
	rm -f *.o $(TESTS)

gens:	../bin/sprotoc *.proto
	@( for i in $(TESTS); do \
	    ../bin/sprotoc --c_out=. $$i.proto; \
	    done; \
	 )

.c.o:
	$(CC) -c -o $@ $(CFLAGS) $^

CFLAGS += -ggdb
LIBS = ../lib/sprotoc.a

TESTS = test1 test_b

test.sh:	run_test.sh $(TESTS)
	@( echo "#!/bin/sh" >test.sh; \
	   echo "cd $(PWD)/test" >>test.sh; \
	   echo "TESTS=\"$(TESTS)\"" >>test.sh; \
	   cat run_test.sh >>test.sh; \
	)

test1:	test1.o test1.pb-c.o $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^

test_b:	test_b.o test_b.pb-c.o $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^


distclean:	clean

clean:
	rm -f *.o $(TESTS)

gens:	$(PWD)/$(SPROTOC) *.proto
	echo $(PWD)/$(SPROTOC)
	@( for i in $(TESTS); do \
	    $(PWD)/$(SPROTOC) --c_out=. $$i.proto; \
	    done; \
	 )

.c.o:
	$(CC) -c -o $@ $(CFLAGS) $^

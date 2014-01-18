OBJS = sprotoc.o generate.o gen_api.o declares.o gens.o helpers.o

../lib/sprotoc-c.a: $(OBJS)
	$(AR) -cr $@ $^

clean:
	rm -f $(OBJS)

.cc.o:	$(PWD)/include
	$(CXX) $(CXXFLAGS) -c -o $@ $^


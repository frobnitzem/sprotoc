OBJS = sprotoc.o generate.o helpers.o

../lib/sprotoc-c.a: $(OBJS)
	$(AR) -cr $@ $^

clean:
	rm -f $(OBJS)

.cc.o:	$(PWD)/include
	$(CXX) $(CXXFLAGS) -c -o $@ $^


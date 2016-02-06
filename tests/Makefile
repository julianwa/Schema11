SOURCES=$(wildcard *.cpp) ../third_party/json11/json11.cpp

CXX=c++
LD=c++
IFLAGS= -I/usr/include/ -I/usr/local/include
LIBS= -L/usr/lib -L/usr/local/lib -lm -lc++ 
#CFLAGS=  -g -D_DEBUG
CFLAGS=   -O3 -std=c++14 -stdlib=libc++

PROGRAM=tests

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
TRANSLATIONUNITS = $(patsubst %.cpp,%.mi,$(SOURCES))
ASMFILES = $(patsubst %.cpp,%.S,$(SOURCES))

all: $(PROGRAM)

asm: $(ASMFILES)

preprocess: $(TRANSLATIONUNITS)

$(PROGRAM) : $(OBJECTS)
	$(LD) $(LIBS) -o $(PROGRAM) $(OBJECTS)

%.o: %.cpp %.h

%.mi: %.cpp %.h

%.S: %.cpp %.h

clean:
	rm -f $(OBJECTS) $(PROGRAM) $(TRANSLATIONUNITS) $(ASMFILES)

#-include $(OBJECTALL:=.d)
#%.d : %.cpp
#	set -e; $(CXX) -MM $(CFLAGS) $< \
#		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
#		[ -s $@ ] || rm -f $@
#
.SUFFIXES:  .o .cpp .mi .S
.cpp.o:
	$(CXX) -c $(IFLAGS) $(CFLAGS) -o $@ $<

# It is sometimes useful to look at preprocessor output
# clang has -E option for this.
.cpp.mi:
	$(CXX) -E $(IFLAGS) $(CFLAGS) -o $@ $<

# This outputs assembly. Note there will be no comdat foldering or lto kinds of optimization
.cpp.S:
	$(CXX) -S $(IFLAGS) $(CFLAGS) -o $@ $<

test: $(PROGRAM)
	./$(PROGRAM)
	
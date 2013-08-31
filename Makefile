INCLUDEDIR = /cse/courses/csep548/12au/MultiCacheSim
OBJDIR = /cse/courses/csep548/12au/MultiCacheSim

TARGETS = MSI_SMPCache.so MESI_SMPCache.so

CXXFLAGS += -I. -g -O0 -Wno-deprecated -I$(INCLUDEDIR)

all: $(TARGETS)

%.so : %.cpp
	$(CXX) -fPIC -shared $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $< $(OBJDIR)/SMPCache.o $(OBJDIR)/Snippets.o $(OBJDIR)/nanassert.o

## cleaning
clean:
	-rm -f *.so

-include *.d

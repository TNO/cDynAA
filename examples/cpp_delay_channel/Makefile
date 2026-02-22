IDIR = ../../cdynaa/include/ 
SDIR = .
ODIR = .
LDIR = .
CXX      = g++ 
CXXFLAGS = -O1 -fPIC -m64 -fopenmp -std=c++17 -MMD -MP
CXXFLAGS += -I. -I$(IDIR)
CXXFLAGS += -Wall -Wextra -Wno-missing-braces -fmax-errors=5
PROFILER = valgrind
PROFILERFLAGS = --tool=callgrind
DFLAGS = -g

DYNAA_LIB = ../../cdynaa/libdynaacore.a

LIBS = -lm 

EXAMPLE = delay_channel

TARGET = run_cpp_$(EXAMPLE)

SRCS = \
	$(SDIR)/$(EXAMPLE).cpp\

OBJS = $(SRCS:$(SDIR)/%.cpp=$(ODIR)/%.o)

$(ODIR)/%.o: $(SDIR)/%.cpp
	@echo "*** Compiling $(TARGET) example"
	$(CXX) $(CXXFLAGS) -c -o $@ $<

default: all
all: $(EXAMPLE)

delay_channel: $(OBJS)
	@echo "*** Linking $(TARGET) example"
	$(CXX) $(CXXFLAGS) $(DFLAGS) -o $(TARGET) $^ $(LIBS) -L. $(DYNAA_LIB)

clean:
	/bin/rm -f $(OBJS) $(OBJS:%.o=%.d)
	/bin/rm -f ./$(TARGET)

profile: $(EXAMPLE)
	$(PROFILER) $(PROFILERFLAGS) ./$(TARGET)
    
	

.PHONY: default all $(EXAMPLE)

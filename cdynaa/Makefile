IDIR = include/
SDIR = src/
ODIR = obj/
LDIR = libs/
TDIR = test/

CXXFLAGS = -O1 -fPIC -std=c++17 -MMD -MP 
CXXFLAGS += -I. -I$(IDIR)
CXXFLAGS += -Wall -Wextra -Wno-unused-parameter
DFLAGS = -g

LIBS = -lm

STATIC_LIB_TARGET = libdynaacore.a
SHARED_LIB_TARGET = libdynaacore.so

DEF_ASTYLE = $(shell astyle --version 2>/dev/null)

SRCS_CORE = \
	$(SDIR)/event.cpp\
	$(SDIR)/eventtype.cpp\
	$(SDIR)/entity.cpp\
	$(SDIR)/defaultscheduler.cpp\
	$(SDIR)/defaulthandlerregistry.cpp\
	$(SDIR)/diagnostics.cpp\
	$(SDIR)/diagnosticseventstorage.cpp\
	$(SDIR)/diagnosticshandlerstorage.cpp\
	$(SDIR)/handlerreport.cpp\
	$(SDIR)/eventhandler.cpp\
	$(SDIR)/simengine.cpp\
	$(SDIR)/eventexpression.cpp\
	$(SDIR)/handlerrecord.cpp\
	$(SDIR)/handlertoken.cpp\
	$(SDIR)/expressionstate.cpp\
	$(SDIR)/cythonutils.cpp\


SRCS_TEST = \
	$(TDIR)/dynaaunittest.cpp\
	$(TDIR)/event_test.cpp\
	$(TDIR)/entity_test.cpp\
	$(TDIR)/defaulthandlerregistry_test.cpp\
	$(TDIR)/diagnosticseventstorage_test.cpp\
	$(TDIR)/diagnosticshandlerstorage_test.cpp\
	$(TDIR)/diagnostics_test.cpp\
	$(TDIR)/simengine_test.cpp\
	$(TDIR)/defaultscheduler_test.cpp\
	$(TDIR)/eventexpression_test.cpp\
	$(TDIR)/handlerrecord_test.cpp\
	$(TDIR)/handlerreport_test.cpp\
	$(TDIR)/eventtype_test.cpp\
	$(TDIR)/handlertoken_test.cpp\

OBJS_CORE = $(SRCS_CORE:$(SDIR)/%.cpp=$(ODIR)/%.o)
OBJS_TEST = $(SRCS_TEST:$(TDIR)/%.cpp=$(ODIR)/%.o)

$(ODIR)/%.o: $(TDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< 

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

default: staticlib runtests
all: staticlib sharedlib tests

staticlib: $(OBJS_CORE)
	@echo -e "\n*** Building static DynAA-core library ($(STATIC_LIB_TARGET))"
	ar -cr $(STATIC_LIB_TARGET) $(OBJS_CORE)

sharedlib: $(OBJS_CORE)
	@echo -e "\n*** Building shared DynAA-core library ($(SHARED_LIB_TARGET))"
	$(CXX) -shared -o $(SHARED_LIB_TARGET) $^

tests: $(OBJS_TEST)
	@echo -e "\n*** Building unit tests for static DynAA-core library ($(STATIC_LIB_TARGET))"
	$(CXX) $(CXXFLAGS) $(DFLAGS) -o run_tests $^ $(LIBS) -L. $(STATIC_LIB_TARGET)

runtests: tests
	@echo -e "\n*** Running all unit tests on DynAA-core library"
	-./run_tests

-include $(OBJS_CORE:%.o=%.d) $(OBJS_TEST:%.o=%.d)

clean:
	/bin/rm -f $(OBJS_CORE) $(OBJS_TEST) $(OBJS_CORE:%.o=%.d) $(OBJS_TEST:%.o=%.d)
	/bin/rm -f ./$(STATIC_LIB_TARGET)
	/bin/rm -f ./$(SHARED_LIB_TARGET)
	/bin/rm -f ./run_tests

style:
	@echo -e "\n*** Styling all files in src, include, and test"
	astyle --style=java --suffix=none --max-code-length=120 $(SRCS_CORE)
	astyle --style=java --suffix=none --max-code-length=120 $(SRCS_TEST)
	astyle --style=java --suffix=none --max-code-length=120 --exclude=$(TDIR)/catch.h "$(TDIR)/*.h"
	astyle --style=java --suffix=none --max-code-length=120 $(IDIR)/*.h

.PHONY: default all staticlib sharedlib tests runtests clean style

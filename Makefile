#CPPUTEST_HOME = ../cpputest-3.8
CPPUTEST_HOME = .
CPPFLAGS += -I$(CPPUTEST_HOME)/include
LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib -lCppUTest
TARGET = main
SOURCES = $(TARGET).c
OBJS = $(TARGET).o
TEST_RUNNER = RUNNER
#OPTIONS = -std=c++11
OPTIONS = 
COMPILER = g++

.SUFFIXES: .cpp

all: $(TEST_RUNNER)

$(TEST_RUNNER):
	@$(COMPILER) -o $@ $(SOURCES) $(CPPFLAGS) $(LD_LIBRARIES) $(OPTIONS)
	@./$@

clean:
	@rm -f $(TEST_RUNNER)

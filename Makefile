CFLAGS		= -O0 -g
CXXFLAGS	= -O0 -g
LDLIBS		= -lm
SOURCES		= $(wildcard *.cpp)
OBJECTS		= $(patsubst .cpp,.o,$(SOURCES))

lines: $(OBJECTS)


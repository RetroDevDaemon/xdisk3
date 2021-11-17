#   Use Gnu Make for this.

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)

CPPFLAGS = -g

xdisk3: $(OBJS)
	g++ $(CPPFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -f $(OBJS)

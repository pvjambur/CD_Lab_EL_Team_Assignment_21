CXX      = g++
CXXFLAGS = -std=c++14 -O1 -Wall
TARGET   = nsan_interactive

all: $(TARGET).exe

$(TARGET).exe: $(TARGET).cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(TARGET).exe

.PHONY: all clean

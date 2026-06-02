CXX      = g++
CXXFLAGS = -std=c++14 -O1 -Wall
LDFLAGS  = -lcomctl32 -lgdi32 -luser32 -mwindows
TARGET   = nsan_gui

all: $(TARGET).exe

$(TARGET).exe: $(TARGET).cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET).exe

.PHONY: all clean

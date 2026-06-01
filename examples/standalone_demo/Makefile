CXX     = g++
CXXFLAGS = -std=c++14 -O1 -Wall

TARGET  = nsan_demo
SRC     = nsan_demo.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

run-verbose: $(TARGET)
	NSAN_VERBOSITY=2 ./$(TARGET)

run-strict: $(TARGET)
	NSAN_REL_EPSILON=1e-8 ./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe

.PHONY: all run run-verbose run-strict clean

CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++11
TARGET   = browser
SRC      = MultitabBrowser__LRU_Simulator.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe

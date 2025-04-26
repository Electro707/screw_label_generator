LDLAGS = -l ptouch
LDLAGS += -lgd -lpng -lz -ljpeg -lfreetype -lm
LDLAGS += `pkg-config --cflags --libs librsvg-2.0`

CFLAGS = -Wall -g
CFLAGS += `pkg-config --cflags-only-I --libs librsvg-2.0`

BUILD_FOLDER = build


main:
	mkdir -p $(BUILD_FOLDER)
	g++ $(CFLAGS) -c imageGen.cpp -o $(BUILD_FOLDER)/imageGen.o
	g++ $(CFLAGS) -c main.cpp -o $(BUILD_FOLDER)/main.o
	g++ -g $(BUILD_FOLDER)/main.o $(BUILD_FOLDER)/imageGen.o -o labelGen.out $(LDLAGS)
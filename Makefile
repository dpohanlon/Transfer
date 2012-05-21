CC=clang
CFLAGS=-Wall -g -O2

SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TARGET=bin/transfer

all: $(TARGET)

$(TARGET): build $(OBJECTS) 
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -rf bin
	rm src/*.o

build:
	@mkdir -p bin

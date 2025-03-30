CC = gcc
CFLAGS = -Wall -g `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

TARGET = codebase-exporter
SOURCE = codebase_exporter.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(SOURCE) $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

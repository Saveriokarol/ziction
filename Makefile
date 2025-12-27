INSTALL = install
CFLAGS = -Wall -std=c99

SRC = ziction.c
TARGET = ziction

$(TARGET): $(SRC)

install: $(TARGET)
	$(INSTALL) -m 755 $(TARGET) /usr/local/bin

clean:
	rm -f $(TARGET) *.o

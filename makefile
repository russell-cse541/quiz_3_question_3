CC = gcc
CFLAGS = -I. -I./memory_allocator

DEPS = memory_allocator/memory_allocator.h
OBJ = test.o memory_allocator/memory_allocator.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test_binary: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJ) test_binary

test: clean test_binary
	./test_binary

test_canary: CFLAGS += -DUSE_CANARIES
test_canary: clean test
	./test_binary
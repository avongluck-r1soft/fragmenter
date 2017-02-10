CC=gcc

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

fragmenter: fragmenter.o
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f *.o fragmenter

CC=clang
CFLAGS=-Wall -Werror -g -fsanitize=undefined,address

kv: kv.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm kv

myio_test: myio.c myio.h myio_test.c
	gcc -Wall -pedantic -o myio_test myio_test.c

.PHONY: clean
clean:
	rm -f myio_test
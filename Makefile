neo: io.o err.o
	gcc $(shell pwd)/src/neo.c ./io.o ./err.o -Wall -Wextra -o ./neo

io.o:
	gcc -Wall -Wextra -c -o ./io.o $(shell pwd)/src/io.c -I$(shell pwd)/include/

err.o:
	gcc -Wall -Wextra -c -o ./err.o $(shell pwd)/src/err.c -I$(shell pwd)/include/

clean:
	rm ./neo ./*.o
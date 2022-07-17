all: send115200

send115200: rs232.c rs232.h send115200.c
	gcc -o send115200 rs232.c rs232.h send115200.c

clean:
	rm *.o send115200

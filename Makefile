TARGET = smarthome
CFLAGS = -std=c99 -O2 -Wall -I./inc

compile:
	mkdir -p obj

	gcc $(CFLAGS) -c src/main.c -o obj/main.o
	gcc $(CFLAGS) -c src/config.c -o obj/config.o
	gcc $(CFLAGS) -c src/log.c -o obj/log.o
	gcc $(CFLAGS) -c src/modules.c -o obj/modules.o
	gcc $(CFLAGS) -c src/utils.c -o obj/utils.o

	gcc -o $(TARGET) obj/main.o obj/config.o obj/log.o obj/modules.o obj/utils.o -ldl -lpthread

clean:
	rm -f obj/*.o

all:
	compile

.DEFAULT_GOAL := all
TARGET = smarthome
CFLAGS = -O2 -Wall -I./inc -I./lib/httpserver -I./vendor/paho.mqtt.c/src
CFLAGS2 = -std=c99 -D _POSIX_C_SOURCE=200809L $(CFLAGS)

OBJS = obj/main.o obj/config.o obj/logger.o obj/messaging.o obj/modules.o obj/utils.o obj/webapi.o\
       obj/MQTTAsync.o obj/MQTTPersistence.o obj/MQTTPersistenceDefault.o obj/MQTTPacket.o obj/MQTTPacketOut.o obj/MQTTProtocolClient.o obj/MQTTProtocolOut.o

core:
	mkdir -p obj

	gcc $(CFLAGS2) -c src/main.c -o obj/main.o
	gcc $(CFLAGS2) -c src/config.c -o obj/config.o
	gcc $(CFLAGS2) -c src/logger.c -o obj/logger.o
	gcc $(CFLAGS2) -c src/messaging.c -o obj/messaging.o
	gcc $(CFLAGS2) -c src/modules.c -o obj/modules.o
	gcc $(CFLAGS2) -c src/utils.c -o obj/utils.o
	gcc $(CFLAGS2) -c src/webapi.c -o obj/webapi.o

lights:
	mkdir -p obj/lights
	mkdir -p modules

	gcc $(CFLAGS2) -fPIC -c mod/lights/main.c -o obj/lights/main.o
	gcc $(CFLAGS2) -fPIC -c mod/lights/light.c -o obj/lights/light.o
	gcc $(CFLAGS2) -fPIC -c mod/lights/manager.c -o obj/lights/manager.o

	gcc -o modules/lights.so obj/lights/main.o obj/lights/light.o obj/lights/manager.o -lm -shared

alarm:
	mkdir -p obj/alarm
	mkdir -p modules

	gcc $(CFLAGS2) -fPIC -c mod/alarm/main.c -o obj/alarm/main.o
	gcc $(CFLAGS2) -fPIC -c mod/alarm/alarm.c -o obj/alarm/alarm.o

	gcc -o modules/alarm.so obj/alarm/main.o obj/alarm/alarm.o -shared

mqtt:
	mkdir -p obj

	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/MQTTAsync.c -o obj/MQTTAsync.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/MQTTPersistence.c -o obj/MQTTPersistence.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/MQTTPersistenceDefault.c -o obj/MQTTPersistenceDefault.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/MQTTPacket.c -o obj/MQTTPacket.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/MQTTPacketOut.c -o obj/MQTTPacketOut.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/MQTTProtocolClient.c -o obj/MQTTProtocolClient.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/MQTTProtocolOut.c -o obj/MQTTProtocolOut.o	
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/Clients.c -o obj/Clients.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/Heap.c -o obj/Heap.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/LinkedList.c -o obj/LinkedList.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/Log.c -o obj/Log.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/Messages.c -o obj/Messages.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/SocketBuffer.c -o obj/SocketBuffer.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/Socket.c -o obj/Socket.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/StackTrace.c -o obj/StackTrace.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/Thread.c -o obj/Thread.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/Tree.c -o obj/Tree.o
	gcc $(CFLAGS) -c vendor/paho.mqtt.c/src/utf-8.c -o obj/utf-8.o


link:
	gcc -o $(TARGET) $(OBJS) obj/Clients.o obj/Heap.o obj/LinkedList.o obj/Log.o obj/Messages.o obj/SocketBuffer.o obj/Socket.o obj/StackTrace.o obj/Thread.o obj/Tree.o obj/utf-8.o -ldl -lpthread -L./lib/httpserver -lhttpserver

clean:
	rm -f obj/*.o obj/lights/*.o

all:
	make core
	make lights
	make alarm
	make link

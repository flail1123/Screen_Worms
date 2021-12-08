
all:
	g++ -g -Wextra -O2 -std=c++17 -c err.c

	g++ -g -Wextra -O2 -std=c++17 -c crc32.cpp

	g++ -g -Wextra -O2 -std=c++17 -c client.cpp

	g++ -g -Wextra -O2 -std=c++17 -c screen-worms-client.cpp

	g++ -g -Wextra -O2 -std=c++17 -o screen-worms-client screen-worms-client.o err.o crc32.o client.o

	g++ -g -Wextra -O2 -std=c++17 -c server.cpp

	g++ -g -Wextra -O2 -std=c++17 -c screen-worms-server.cpp

	g++ -g -Wextra -O2 -std=c++17 -o screen-worms-server screen-worms-server.o err.o crc32.o server.o
clean:
	rm err.o
	rm crc32.o
	rm server.o
	rm client.o
	rm screen-worms-server.o
	rm screen-worms-client.o
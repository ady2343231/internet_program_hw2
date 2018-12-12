hw2:*.c
		gcc -pthread -o server chat_server.c
		gcc -pthread -o 1/client chat_client.c
		gcc -pthread -o 2/client chat_client.c
		gcc -pthread -o 3/client chat_client.c
clean:
		rm server
		rm 1/client
		rm 2/client
		rm 3/client

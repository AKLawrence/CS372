Amanda Lawrence
November 27, 2019
CS 372 - Intro to Computer Networks
Project 2
readme.txt


- Run the server on one flip server(I used flip1), and the client on another flip server(I used flip2). 

- the compile the server, type   gcc -o ftserver ftserver.c  in one terminal window. Then, run the server by typing in ./ftserver <SERVER_PORT>

- to run the client program, give it executable permissions, by using chmod +x ftclient.py and then, type:
	 python3 ftclient.py <SERVER_NAME> <SERVER_PORT> <COMMAND> <OPTIONAL_FILENAME> <CLIENT_PORT>

- In order to accommodate the invalid command condition, the ftclient.py program can accommodate either format: 
	python3 ftclient.py <SERVER_NAME> <SERVER_PORT> <INVALID_COMMAND> <FILENAME> <CLIENT_PORT>
	python3 ftclient.py <SERVER_NAME> <SERVER_PORT> <INVALID_COMMAND> <CLIENT_PORT>
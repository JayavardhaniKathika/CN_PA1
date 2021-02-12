Programming Assignment 1:
Client using customized protocol on top of UDP protocol for sending information to the server.
One client connects to one server.

Pre-requisite:
Make sure you have gcc compiler

How to compile and run the files in mac:

1. Copy 'clientudp.c', 'serverudp.c' into a file.
2.Change the current path to the location of this file. (Use pwd to find current location and change to the desired file location using cd)
3. Run the below commands to compile the C programs:
	gcc clientudp.c -o client
	gcc serverudp.c -o server
4. Use command+d to open terminal windows side by side. Run the server first on left side window. To start the server, use the below command:
	./server
5. Run client on right side. To run the client use:
	./client
6. Packets start transmitting and output will be displayed






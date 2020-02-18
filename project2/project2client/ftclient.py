# Amanda Lawrence
# November 27, 2019
# CS 372 - Intro to Computer Networks
# Project 2
# ftclient.py
#
# Implement 2-connection client-server network application
# Practice using the sockets API
# 
# Design and implement a simple file transfer system, i.e., create a file transfer server and a file transfer client
# Write the ftserver and ftclient programs.
# The final version of your programs must accomplish the following tasks:
#  1. ftserver starts on Host A and validates command-line parameters (<SERVER_PORT>)
#  2. ftserver waits on <PORTNUM> for a client request
#  3. ftclient starts on Host B, and validates any pertinent command-line parameters.
#  4. ftserver and ftclient establish a TCP control connection on <SERVER_PORT>. 
#      (For the remainder of this description, call this connection P)
#  5. ftserver waits on connection P for ftclient to send a command.
#  6. ftclient sends a command (-l (list) of -g <FILENAME> (get)) on connection P.
#  7. ftserver receives command on connection P.
#      If ftclient sent an invalid command:
#	       - ftserver sends an error message to ftclient on connection P, and ftclient displays the message on-screen. 
#      Otherwise:
#		   - ftserver initiates a TCP data connection with ftclient on <DATA_PORT> (called connection Q)
#		   - If ftclient has sent the -l command, ftserver sends its directory to ftclient on connection Q,
#				and ftclient displays the directory on-screen.
#		   - If ftclient has sent -g <FILENAME>, ftserver validates FILENAME, and either:
#					- sends the contents of FILENAME on connection Q. ftclient saves the file in the current
#						default directory (handling "duplicate file name" error if necessary), and displays a
#						"transfer complete" message on-screen.
#					- Or, sends an appropriate error message ("File not found", etc.) to ftclient on connection
#						P, and ftclient displays the message on-screen.
#		   - ftserver closes connection Q (don't leave open sockets!)
# 8. ftclient closes connection P (don't leave open sockets!) and terminates.
# 9. ftserver repeats from 2 (above) until terminated by a supervisor (SIGINT).
#


# The official Python documentation for sockets was used to create this program: https://docs.python.org/2/howto/sockets.html


import sys
import socket
import os.path
import time


def createDataSocket():
	if sys.argv[3] == "-l":
		clientPort = sys.argv[4]
	elif sys.argv[3] == "-g":
		clientPort = sys.argv[5]
	else:
		printf("Please enter command -l for list of directory contents, or, -g for a file")
	#portNumber = sys.argv[1]		# port this server is run on, specified by user
	serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)		# the socket for use by server on portNumber
	serverSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	#print("About to bind")

	serverSocket.bind(('', int(clientPort)))			# bind our socket to our portNumber. Expects tuple.
	#print("About to listen")
	serverSocket.listen(5)			# start listening for messages
	#print("About to accept")
	conn, address = serverSocket.accept()
	return conn



def connect():
	if sys.argv[1] == "flip1" or sys.argv[1] == "flip2" or sys.argv[1] == "flip3":
		serverName = sys.argv[1]+".engr.oregonstate.edu"
	else:
		serverName = sys.argv[1]
	# serverName = sys.argv[1]+".engr.oregonstate.edu"
	serverPort = int(sys.argv[2])
	connectionSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	connectionSocket.connect((serverName, serverPort))
	return connectionSocket


# This will receive the list of filenames from server. We loop over filenames until we receive the "complete" message
def receiveList(clientSocket):
	rsize = 4096
	maxIterations = 10
	counter = 0
	theList = ''
	newData = clientSocket.recv(rsize)
	while len(newData):
		theList += newData.decode('utf-8')
		newData = clientSocket.recv(rsize)
		counter += 1
		#print("iteration %d ... newData: '%s'" % (counter, newData))

	if not theList.endswith('complete'):
		sys.stderr.write("Incomplete data transfer!!\n")
		sys.exit(1)
	#print("theList: '%s'" % theList)
	files = theList.split('\n')[:-1]
	#print("files: '%s'" % files)
	print('\n'.join(files))
	return 









def receiveFile(clientSocket, filename):
	rsize = 4096
	print("Receiving file '%s' from the server" % filename)
	# check if the file exists in the client folder already, as we'll need to handle duplicate file names
	if os.path.exists(filename):
		print("File by the name '%s' was also found on the client side." % filename)
		print("Please rename one of these files.")
		return
	with open(filename, 'w') as ff:
		time.sleep(0.0001)
		newData = clientSocket.recv(rsize)

		while len(newData):
			ff.write(newData.decode())
			newData = clientSocket.recv(rsize)
	return










# This function for retrieving IP address from: https://stackoverflow.com/questions/24196932/how-can-i-get-the-ip-address-of-eth0-in-python
def getIPAddress():
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	s.connect(("8.8.8.8", 80))
	return s.getsockname()[0]


def exchangeData(ctrlSocket):
	if sys.argv[3] == "-l":
		print("Requesting directory structure from ", sys.argv[1], ":", sys.argv[2])
		ctrlSocket.send("-l".encode('utf-8'))
		ack = ctrlSocket.recv(2048)
		#print("command ack: %s" % ack)
		ctrlSocket.send(sys.argv[4].encode('utf-8')) #client port
		ack = ctrlSocket.recv(2048)
		#print("port ack: %s" % ack)
		ctrlSocket.send(getIPAddress().encode('utf-8'))
		ack = ctrlSocket.recv(2048)
		#print("ipaddr ack: %s" % ack)
		#print("Message is: '%s'" % message)
		ctrlSocket.send("proceed".encode('utf-8')) # used to resolve race conditions
		#print("proceed sent, waiting for result.")
		result = ctrlSocket.recv(2048)
		dataSocket = createDataSocket()
		if not dataSocket:
			print("dataSocket not set correctly")
			print(str(dataSocket))
			sys.exit(1)
		receiveList(dataSocket)
		dataSocket.close()
	elif sys.argv[3] == "-g":
		print("Requesting ", sys.argv[4], " from ", sys.argv[1])
		ctrlSocket.send("-g".encode('utf-8'))
		ctrlSocket.recv(2048)
		ctrlSocket.send(sys.argv[5].encode('utf-8')) #client port
		ctrlSocket.recv(2048)
		ctrlSocket.send(getIPAddress().encode('utf-8'))
		message = ctrlSocket.recv(2048)
		#print("Received message, after IP: '%s'" % message)
		ctrlSocket.send(sys.argv[4].encode('utf-8')) # filename
		statusmessage = ctrlSocket.recv(2048)
		filename = sys.argv[4]
		#print("Received filename ack: '%s'" % statusmessage)
		ctrlSocket.send("proceed".encode('utf-8')) # used to resolve race conditions
		#print("proceed sent, waiting for result of file check.")
		result = ctrlSocket.recv(2048)
		#print("Received result message, about filename: '%s'" % result)
		if result != b"ok": #The file was not found, we display FILE NOT FOUND and return
			print("%s: %s says %s" % (sys.argv[1], sys.argv[2], result.decode()))
			return
		# We continue below if the file was found
		dataSocket = createDataSocket()
		if not dataSocket:
			print("dataSocket not set correctly.")
			print(str(dataSocket))
			sys.exit(1)
		#dataSocket = None
		receiveFile(dataSocket, filename)
		print("File transfer complete.")
		dataSocket.close()
	else: #in the case that the client received an invalid command, it will need to receive error from server and display it. 
		#print("The client user has entered an invalid command.")
		ctrlSocket.send("bad".encode('utf-8'))
		ctrlSocket.recv(2048)

		portNum = 0;
		if (sys.argv[5] != 0):
			portNum = sys.argv[5]
		else:
			portNum = sys.argv[4]


		ctrlSocket.send(portNum.encode('utf-8')) #client port
		ctrlSocket.recv(2048)
		ctrlSocket.send(getIPAddress().encode('utf-8'))
		message = ctrlSocket.recv(2048)

		ctrlSocket.send("proceed".encode('utf-8')) # used to resolve race conditions

		receiveError = ctrlSocket.recv(2048)	# Receive error message from server
		print(receiveError.decode())


		#ctrlSocket.send(getIPAddress().encode('utf-8'))
		#message = ctrlSocket.recv(2048)
		#dataSocket = createDataSocket()
		#receiveFile(dataSocket, filename)
		#dataSocket.close()
	return




if (len(sys.argv) > 6 or len(sys.argv) < 5):
	print("Please include: server (flip1, flip2, flip3), server port number, command (-l, -g), and client port number.")
	exit(1)
else:
	newSocket = connect()
	exchangeData(newSocket)
	sys.exit(0)










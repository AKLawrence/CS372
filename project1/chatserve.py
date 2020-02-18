# Amanda Lawrence
# Last Modified: November 3, 2019 - 5:30 PM HST
# CS 372 - Intro to Computer Networks
# Project 1
# chatserve.py
#
# Implement a client-server network application
# Learn to use the sockets API
# Use the TCP protocol
# 
# chatserve starts on host A, and waits on a port (specified at the command-line) for a client request.
# chatclient starts on host B, specifying host A's hostname and port number on the command line. 
# chatclient on host B gets the user's "handle" by initial query (a one-word name, up to 10 characters). 
# the chatclient will display this handle as a prompt on host B and will prepend it to all messages sent to host A. ("SteveO> Hi")
# chatclient on host B sends an initial message to chatserve on host A: PORTNUM. This causes a connection
# to be established between host A and host B. Host A and host B are now peers, and may alternate sending and receiving messages. 
# Responses from host A should have host A's "handle" prepended.
# Host A responds to host B, or closes the connection with the command "\quit"
# Host B responds to host A, or closes the connection with command "\quit"
# If the connection is not closed at this point, host A and host B should still be able to respond to the other.
# However, if the connection is closed, chatserve repeats from 2 (until a SIGINT is received)
#
# Programs must be able to send messages up to 500 characters
# Don't hard-code any directories


import sys
import socket
import atexit
import signal
import os


# Adjusts the write() function to both write() and flush()
class Unbuffered(object):
	def __init__(self, stream):
		self.stream = stream
	def write(self, data):
		self.stream.write(data)
		self.stream.flush()
	def __getattr__(self, attr):
		return getattr(self.stream, attr)

sys.stdout = Unbuffered(sys.stdout)
sys.stderr = Unbuffered(sys.stderr)


# Adjusts the signal handler to kill the server process, as ctrl+C was not sufficiently closing down server process.
def signal_handler(signum, frame):
	sys.stderr.write("\nClosing Server down\n")
	os.kill(os.getpid(), signal.SIGTERM)
	sys.exit(1)

signal.signal(signal.SIGINT, signal_handler)


def exchangeMessage(socketFD, clientname, servername):
	while(1):
		#sys.stderr.write("Waiting for message ... ")
		raw_response = socketFD.recv(501).decode('UTF-8')
		#sys.stderr.write("received.\n")
		received = raw_response.rstrip("\r\n")
		#sys.stderr.write(":--> raw_response: '%s'\n" % raw_response)
		#sys.stderr.write(":--> received:     '%s'\n" % received)
		#import pdb; pdb.set_trace()
		# check the socket contains something and continue setting up connection, else, close connection.
		
		if len(raw_response) == 0:
			sys.stdout.write("Connection closed.\n")
			return

		if len(received) > 0:
			#received = received.rstrip("\r\n")								# strips trailing newline chars from the end of this string
			##print("{}> {}".format(clientname, received))					# prints out the client user's name and the message they sent
			sys.stdout.write("%s\n" % received)
			#message_to_send = "%s> " % servername											# Message we'll send to the client
			message_to_send = raw_input("{}> ".format(servername))
			#message_to_send += input()
			#if len(message_to_send) == 0 or len(message_to_send) > 500:
							# Note: Python 2 uses raw_input(), Python 3 uses input()
			if message_to_send == "\quit":
				socketFD.send("quit".encode())
				print ("Connection closed.")
				return
			socketFD.send(message_to_send[:500].encode())



def handshake(socketFD, servername):
	# receive the client username, and send the servername from the server.
	clientname = socketFD.recv(1024).decode('UTF-8')
	#sys.stderr.write("in handshake, got clientname: '%s'\n" % clientname)
	#sys.stderr.write("checking bbuffer again...\n")
	#leftover = socketFD.recv(1024).decode('UTF-8')
	#sys.stderr.write("leftover: '%s'\n" % leftover)
	socketFD.send(servername.encode())

	return clientname


if len(sys.argv) != 2:
	print("Please include a port number to run this server on")
	#quit()
	sys.exit()
else:
	servername = ""
	#while (len(servername) < 10):
	servername = raw_input("What is your username? ")		# Note: Python 2 uses raw_input(), Python 3 uses input()
	print("The server is ready for messages from the client.")


portNumber = sys.argv[1]		# port to run this server on, specified by user
serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)		# the socket for use by server on portNumber, uses default address family and TCP.
serverSocket.bind(('', int(portNumber)))			# bind our socket to our portNumber. Expects tuple.

@atexit.register
def cleanup():
	serverSocket.close()
	return

while 1:
	try:
		serverSocket.listen(1)			# start listening for messages from client
		conn, addr = serverSocket.accept()
		clientname = handshake(conn, servername)
		sys.stderr.write("You have connected with:  '%s'\n" % clientname)
		#import pdb; pdb.set_trace()
		#serverSocket.send(servername.encode())
		exchangeMessage(conn, clientname, servername)
		#serverSocket.close()
	except:
		sys.stderr.write("exception occurred!\n")





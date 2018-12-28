from socket import *
import sys

'''
This program implements the server side of CS456 Programming Assignment 1: Introductory Socket Programming.

The communication process between server and client contains 2 stages: Negotiation Stage and Transaction Stage.

In the Negotiation Stage, the server creates a UDP socket with a random port <n_port> and starts listening on <n_port>.
The server prints the <n_port> to the standard output.
The client also creates a UDP socket with the server using serverAddress and <n_port>.
The client sends a <req_code> to the server to get the random port number <r_port> for later use in the transcation stage.
The server receives the UDP request with the user-specified <req_code> from the client.
The server verifies the <req_code>.
    If the client fails to send the intended <req_code>, the server does nothing.
    If successfully verified, the server creates a TCP socket with a random unused <r_port> and replies back with <r_port> using UDP socket.
The server confirms the <r_port> is successfully sent to the client and sends back an acknowledgement.
The client receives the acknowledgement, and closes its UDP socket.

In the Transaction Stage, the client creates a TCP connection to the server at <r_port>.
The server receives a string from the client, prints it to the standard output along with its TCP port number <r_port>, 
    reverses this string, and sends the reversed string back to the client.
The client receives the reversed string and prints it to the standard output.

After the TCP session, the server continues to listen on <n_port> for future sessions from new clients.
In this assignment, the server can only handle 1 client at a time.

Example execution:
Run server: python server.py <req_code>

<req_code>: int, user-specified.
'''

# check input parameters number and type
if len(sys.argv) == 1:
    print('Parameter <req_code> missing!')
    exit(-1)
    
elif not len(sys.argv) == 2:
    print('Should have only 1 parameter <req_code>!')
    exit(-1)

try:
    req = int(sys.argv[1])
except ValueError:
    print('<req_code> should be an integer!')
    exit(-2)

    
### Negotiation Stage: ### 
# 1. the server creates a UDP socket with a random <n_port>
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.bind(('', 0))
serverAddress, n_port = serverSocket.getsockname()
print('SERVER_PORT=%d' % n_port)
# 2. the server starts 'listening' on <n_port>
while True:
    # 3. the server verifies the req_code sent by the client
    req_code, clientAddress = serverSocket.recvfrom(2048)
    req_code = int(req_code)
    if not req_code == req:
        print('Incorrect req_code sent from client: %d' % req_code)
        continue
    # 4. the server creates a TCP socket with a random port number <r_port>
    connectionSocket = socket(AF_INET, SOCK_STREAM)
    connectionSocket.bind(('', 0))
    connectionSocket.listen(1)
    r_port = connectionSocket.getsockname()[1]
    print('SERVER_TCP_PORT=%d' % r_port)
    # 5. the server replies back with <r_port> using UDP socket
    serverSocket.sendto(str(r_port).encode(), clientAddress)
    # 6. the server confirms <r_port> is correctly sent
    r_port_fromClient = serverSocket.recv(2048).decode()
    r_port_fromClient = int(r_port_fromClient)
    if not r_port_fromClient == r_port:
        print('Incorrect r_port sent from client: %d' % r_port_fromClient)
        continue
    # 7. the server acknowledges <r_port> is correctly sent
    serverSocket.sendto('correct'.encode(), clientAddress)
    
    ### Transaction Stage ###
    # 8. the server receives the msg string, prints it, converts it, then sends back the modified msg
    transmissionSocket, Addr = connectionSocket.accept()
    msg = transmissionSocket.recv(2048).decode()
    print('SERVER_RCV_MSG=%s' % msg)
    modifiedMsg = msg[::-1]
    transmissionSocket.send(modifiedMsg.encode())
    

    
          
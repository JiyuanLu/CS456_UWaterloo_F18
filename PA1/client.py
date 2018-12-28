from socket import *
import sys

'''
This program implements the client side of CS456 Programming Assignment 1: Introductory Socket Programming.

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
Run client: python client.py <server_address> <n_port> <req_code> <msg>

<server_address>: string, the IP address of the server
<n_port>: int, the negotiation port number of the server
<req_code>: int, user-specified.
<msg>: string, a string that you want to reverse, should be contained in one quote '' or ""
'''
# check input parameters number and type
if sys.argv[1] == '':
    print('Parameters <server_Address>, <n_port>, <req_code>, <msg> missing!')
    exit(-1)    
elif not len(sys.argv) - 1 == 4:
    print('Should have 4 parameters <server_Address>, <n_port>, <req_code>, <msg>!')
    exit(-1)

serverAddress= str(sys.argv[1])
try:
    n_port = int(sys.argv[2])
except ValueError:
    print('<n_port> should be an integer but got %s!' % type(sys.argv[2]))
    exit(-2)
try:
    req_code = int(sys.argv[3])
except ValueError:
    print('<req_code> should be an integer but got %s!' % type(sys.argv[3]))
    exit(-2)
msg = sys.argv[4]
if msg == '':
    print('msg should not be empty!')
    exit(-3)

### Negotiation Stage: ###
# 1. the client creates a UDP socket
clientUDPSocket = socket(AF_INET, SOCK_DGRAM)
# 2. the client sends the req_code to the server to get <r_port>
clientUDPSocket.sendto(str(req_code).encode(), (serverAddress, n_port))
# 3. the client receives <r_port>
r_port = int(clientUDPSocket.recv(2048).decode())
# 4. the client sends back <r_port>
clientUDPSocket.sendto(str(r_port).encode(), (serverAddress, n_port))
# 5. the client receives the ack for <r_port> from server
ack = clientUDPSocket.recv(2048).decode()
if not ack == 'correct':
    print('Incorrect r_port sent by client!')
    sys.exit(1)
# 6. the client closes the UDP socket
clientUDPSocket.close()

### Transaction Stage ###
# 7. the client creates a TCP connection with the server
clientTCPSocket = socket(AF_INET, SOCK_STREAM)
clientTCPSocket.connect((serverAddress, r_port))
# 8. the client sends the msg to the server
clientTCPSocket.send(msg.encode())
# 9. the client receives the modified msg fro the server
modifiedMsg = clientTCPSocket.recv(2048).decode()
# 10. the client prints out the reversed string and then exits
print('CLIENT_RCV_MSG=%s' % modifiedMsg)
sys.exit(0)
# Email system

### The server application must implement the following functions:

1.  Listening to a specific port
2.  Processing requests for connections on this port from clients
3.  Support for simultaneous operation of multiple mail clients through the threading mechanism
4.  Receiving mail from one client to another
5.  Storing email for clients
6.  Sending a mail message to the client upon request, followed by deleting the message
7.  Sending information about the state of the post box to the client
8.  Client disconnect request processing
9.  Force client disconnection

### The client application must implement the following functions:

1.  Establishing a connection to the server
2.  Sending email to server for another client
3.  Checking the status of your mailbox
4.  Receiving a specific letter from the server
5.  Break the connection
6.  Handling the client disconnection situation by the server

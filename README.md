# Tcp-chat-c

																   TEST REPORT:-
																   ------------
=======================================================================================================================

=============================================:-
 THE REDHAT CHAT APPLICATION:-
=============================================:-

* This is a client server application where multiple clients can connect to the server and can chat with each other.
* Server can support only 5 clients to connect.
* As a client connects, a random id, random name with time stamp value is given to the client.
* clients can:-
	1. Query server to display all connected clients.
	2. Send message to other clients using their name.
	3. Broadcast messages to all the clients connected at that instance.
	4. Disconnect itself from the server.
* Whenever a client disconnects it informs all other clients and the server apriori.
* If in case server disconnects , it will disconnect all its associated clients.
	
-----------------------------
 CLIENT'S FUNCTIONALITIES:-
-----------------------------
* To send message the client has to follow a particular format and that is
			<client name>:<msg>
* To broadcast messages the client has to provide message in the format
			broadcast:<msg>
* To Query all connected clients, the client has to use the command
			showUsers
* To disconnect itself from the server, client has to press
			ctrl+c
			
================================================:-
 IMPLEMENTATION DETAILS:-
================================================:-

* CLIENT'S SIDE:
----------------
	* Uses poll system call to check that if there is an incomming message then is it comming from server or the client(user).
		(poll system call is similar to select system call)
	* If the incomming message is from terminal(i.e client) the send the meaasage to the server.
	* Else if message if from the server then just print the incomming message.
	
* SERVER'S SIDE:
----------------
	* Server will fork a new process for each client that is getting connected.
	* Server keeps on checking the message queue and delivers to the client if there is message for its client.
	* If client sends any data to server , server will fperform tasks accordingly
		* Display connected clients to the client(showUsers).
		* Send message other clients
		* Broadcast messages
		* broadcast message if any client gets disconnected.
		(sending messages implies enqueuing messages to the message queue.)

===========================================================================================================================
===========:-
TEST CASES:-
===========:-

--------------------------------------------------------------------------------
| TEST CASE ID			| TEST CASE                                                |
--------------------------------------------------------------------------------
|	1					        | Trying to connect when the server is not running.		     |
|	2					        | Server disconnected in between.                          |
|	3					        | No client name provided by the user.                     |
|	4					        | Client name provided is no more connected.               |
|	5					        | Client name provided doesn't exists.                     |
|	6					        | Colon(:) not provided after the client's name.           |
|	7					        | Client messaging to itself.                              |
|	8					        | Broadcasting meassages to all connected clients.         |
|	9					        | More than 5 clients trying to connect to the server.     |
|	10				        | Port number already in use.                              |
|	11				        | Query ShowUsers when no users are connected.             |
|	12				        | Client Disconnects     								                   |
--------------------------------------------------------------------------------

Below are the test cases which were taken care of:-

TEST CASES:-

------------------------------------------------------------------------------------------------------------------------------------------------------------------
| TestCase Id  | INPUT			| EXPECTED OUTPUT						                   | ACTUAL OUTPUT				                                | RESULT |
------------------------------------------------------------------------------------------------------------------------------------------------------------------
|	1		   | void			| Connection Refused.		                               | Connection Refused.                                        | PASSED |
|	2		   | void			| Server Disconnected.                                     | Server Disconnected.                                       | PASSED |
|	3		   | message		| Please provide a client name.                            | Please provide a client name.                              | PASSED |
|	4	       | client1:msg	| The client is no more connected.                         | The client is no more connected.                           | PASSED |
|	5		   | xyz:msg		| The client doesn't exists.                               | The client doesn't exists.                                 | PASSED |
|	6		   | client1msg		| Please provide input in this format <client_name>:<msg>. | Please provide input in this format <client_name>:<msg>.   | PASSED |
|	7		   | client1:msg	| You cannot ping yourself..                               | You cannot ping yourself.                                  | PASSED |
|	8		   | broadcast:msg	| <clientName>: Message.                                   | <clientName>: Message.                                     | PASSED |
|	9		   | void			| Connection Limit Exceeded.                               | Connection Limit Exceeded.                                 | PASSED |
|	10		   | void			| Connection Refused.                                      | Connection Refused.                                        | PASSED |
|	11		   | showUsers		| No other client connected.                               | No other client connected.                                 | PASSED |
|	12		   | ctrl+c			| <client_name>: Is Disconnected.      					   | <client_name>: Is Disconnected.						    | PASSED |
------------------------------------------------------------------------------------------------------------------------------------------------------------------

===================================================================================================================================================================


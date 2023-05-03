<h1>Winsock Networking Chat Room</h1>
This is a chat room project developed using the Winsock API, featuring a client/server architecture with support for both TCP and UDP protocols. The server uses a non-blocking multiplexed TCP socket and broadcasts TCP information using UDP every second. The client connects to the server through TCP after receiving the UDP broadcast. Additionally, the client is multithreaded, with the TCP receiver running on a detached thread and the TCP sender running on the main thread.

<h3>Installation</h3
<ul>
<li>To run this project on your local machine, you need to have Visual Studio installed.</li>
<li>Clone the repository and open the solution file in Visual Studio.</li>
<li>Build the project and run the server application, followed by the client application.</li>
<li>Make sure that the server application is running before starting the client application.</li>
</ul>

<h3>Usage</h3>
<ul>
<li>After running the client application, you will be prompted to enter a username.</li>
<li>Once you have entered a username, the client will connect to the server using TCP protocol.</li>
<li>You will then be able to join the chat room and start sending messages to other users.</li>
</ul>

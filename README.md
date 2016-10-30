# README #

TCP Server responding to given (and sometimes broken) client. Task for subject Computer Networks at Faculty of Information Technologies at CTU Prague.

### More info about homework:  ###

Robot moves across city. The city consists of 2-D panels with (x,y) coordinates. There are no physical obstacles in the city. The robot can go forth, turn to the left or pick up flag. It is also capable of informing about its city position.

The robot can move just across the city panels, if it gets out of the city it crashes immediately. The city dimensions are 37 x 37. Both these coordinates (x and y) must be within <-18,18> range.

There is a flag at the (0,0) position. The robot's mission is to get at the flag's position, pick it up and read the secret text.

### General scheme of communication ###

The server represents the robot, the robot is to be controlled by client. We take advantage of the TCP protocol. The server listens on the chosen port, the port must be within <3000, 3999>. The protocol is text-oriented and can be tested using command telnet server_address 3998 (in this case using the port 3998).

After the client connects to the server, the robot's coordinates and orientation are generated randomly by the server. The robot's coordinates must be within <-17,17> interval for both x and y. The client must figure out the coordinates of the robot. The client cannot know the initial robot's coordinates or its orientation, so it must move the robot to get this kind of information (e.g. turn the robot, do 1 step and then compare the data). The client must navigate the robot at the flag's place. Then the flag may be picked up. Once it is done the server reveals the secret text and closes the connection. If the flag is not there, the server reports error and also closes the connection (thus the client has only one change to pick the flag up).

The server must send the identification text (ended with \r\n letters – see the code 210 in answer list) immediately after the connection is established. Then it just awaits client's commands and sends the relevant answers.

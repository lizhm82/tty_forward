tty_forward
===========

serial devices forwarding under linux




THIS TOOL CAN BE REPLACED BY "socat" combined with "stty", such as :

// configure serial device as 115200bps, 8 data bits, 1 stop bit, none parity, 
// no hardware flow, no software flow, on echo
$ sudo stty ispeed 115200 ospeed 115200 cs8 -cstopb -parenb -crtscts -ixon -ixoff -ixany -echo raw -F /dev/ttyS0
$ sudo stty ispeed 115200 ospeed 115200 cs8 -cstopb -parenb -crtscts -ixon -ixoff -ixany -echo raw -F /dev/ttyS1

// connect serial devices stream
$ socat open:/dev/ttyS0 open:/dev/ttyS1



#!/usr/bin/perl -w

use strict;
use Socket;

# Set the hostname and port number
# For this sample we'll use the same port number
# that is specified in SampleD.c
my $destination = sockaddr_un("/tmp/plcd");

# Create the socket, connect to the port
socket(SOCKET, PF_UNIX, SOCK_STREAM, 0) || die "socket: $!";
connect(SOCKET, $destination) || die "connect: $!";

# Read whatever the streamm provides
my $message;
while ($message = <SOCKET>) {
        print "$message";
}
close SOCKET || die "close: $!";

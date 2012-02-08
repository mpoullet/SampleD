#!/usr/bin/perl -w

use strict;
use Socket;

# Set the hostname and port number
# For this sample we'll use the same port number
# that is specified in SampleD.c
my $destination = sockaddr_in(12345, inet_aton('localhost'));

# Create the socket, connect to the port
socket(SOCKET, PF_INET, SOCK_STREAM, getprotobyname('tcp')) || die "socket: $!";
connect(SOCKET, $destination) || die "connect: $!";

# Read whatever the streamm provides
my $message;
while ($message = <SOCKET>) {
        print "$message\n";
}
close SOCKET || die "close: $!";

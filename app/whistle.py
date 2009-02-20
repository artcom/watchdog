#!/usr/bin/python
import sys
from socket import *
from socket import SOL_SOCKET, SO_BROADCAST
UDP_PORT=2346

if (len(sys.argv) < 2):
    print "Usage: %s <string> - broadcasts <string> to port %d" % (sys.argv[0], UDP_PORT)
    sys.exit(1)

s = socket(AF_INET, SOCK_DGRAM)
s.bind(('', 0))
s.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
s.sendto(sys.argv[1], ('<broadcast>', UDP_PORT))
print "broadcast '%s' to port %d" % (sys.argv[1], UDP_PORT)

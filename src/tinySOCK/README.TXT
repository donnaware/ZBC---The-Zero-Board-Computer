TinySOCK TCP/IP API for ZBC
=====================================================================

TinySOCK is a very simple implementation of TCP/IP & FTP, suitable 
small computers such as ZBC running DOS. There is no atempt to 
implement multitasking or anything fancy here, this is very stream
lined and is intended to show how to use the 10BaseT interface on 
ZBC. There are other SOCKS type of implementations for DOS that
could probably also be adapted, this one however, is small and
simple and good for learning from. It also includes a small HTTP
server.

I borrowed heavily from TinyTCP by Geof Cooper and  R. Rodman, I just 
added a few things like better arp, icmp, udp, http and a few other 
whizzeys.

Obligatory license crap
---------------------------------------------------------------------
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.
---------------------------------------------------------------------


Source files:
---------------------------------------------------------------------
sed.c      Simple Ethernet Driver - Driver for the ZBC 10BaseT 
sed.h      Interface.

comdrv.c   This is a comm driver for the 10BaseT to RS232 converter. 
comdrv.h   It is unreleated to ZBC except it is good for interfacing
           Ethernet to any computer with an RS232 port.

tinysock.c Has demo programs for tiny sock.
tinysock.h Header file for everything.
proto.h    Function Prototype for everything.
options.h  Put your static IP in here 

Main.c     Main program, here it just calls tinysock.c

arp.c      Simple implementation of Address Resolution Protocol.  

tinyudp.c  Simple implementation of User datagram protocol

tinyicmp.c Simple implementation of ip control message protocol

tinytcp.c  Implementation of TCP.

tinyftp.c  Implementation of FTP, only allows files to be retrieved, not sent.

tinyhttp.c Super simple web server.

---------------------------------------------------------------------


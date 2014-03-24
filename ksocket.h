//*****************************************************************************
//    Copyright (C) 2014 ZenoTec LLC (http://www.zenotec.net)
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//    File: ksocket.h
//    Description: This is a trimmed down version of ksocket to support only
//                 sending UDP packets to a specified host (listener).
//
//*****************************************************************************

#ifndef __KSOCKET_H__
#define __KSOCKET_H__

//*****************************************************************************
// Type definitions
//*****************************************************************************
struct socket;
struct sockaddr;
struct in_addr;
typedef struct socket * ksocket_t;

//*****************************************************************************
// Function prototypes
//*****************************************************************************
ksocket_t ksocket( int domain, int type, int protocol );
int kclose( ksocket_t socket );
ssize_t ksendto( ksocket_t socket, void *message, size_t length, int flags,
                   const struct sockaddr *dest_addr, int dest_len );
unsigned int inet_addr( char *ip );
char *inet_ntoa( const struct in_addr *in ); /* DO NOT forget to kfree the return pointer */

#endif

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
//    File: ksocket.c
//    Description: This is a trimmed down version of ksocket to support only
//                 sending UDP packets to a specified host (listener).
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/kernel.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <asm/processor.h>
#include <asm/uaccess.h>

#include "rtap-ko.h"
#include "ksocket.h"

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************
ksocket_t ksocket( int domain, int type, int protocol )
{
    struct socket *sk = NULL;
    int ret = 0;
	
    ret = sock_create( domain, type, protocol, &sk );
    if ( ret < 0 )
    {
        return( NULL );
    } // end if

    return( sk );
}

//*****************************************************************************
int kclose( ksocket_t socket )
{
    struct socket *sk;
    int ret;

    sk = (struct socket *)socket;
    ret = sk->ops->release( sk );

    if ( sk )
    {
        sock_release( sk );
    } // end if

    return( ret );
}

//*****************************************************************************
ssize_t ksendto( ksocket_t socket, void *message, size_t length, int flags,
                 const struct sockaddr *dest_addr, int dest_len )
{
    struct socket *sk;
    struct msghdr msg;
    struct iovec iov;
    int len;
    mm_segment_t fs;

    sk = (struct socket *)socket;

    iov.iov_base = (void *)message;
    iov.iov_len = (__kernel_size_t)length;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    msg.msg_flags = flags;
    if ( dest_addr )
    {
        msg.msg_name = (void *)dest_addr;
        msg.msg_namelen = dest_len;
    } // end if

    fs = get_fs();
    set_fs( KERNEL_DS );
    len = sock_sendmsg( sk, &msg, length );
    set_fs( fs );
	
    return( len );
}

//*****************************************************************************
unsigned int inet_addr( const char *ip_ )
{
    int a, b, c, d;
    char addr[4];
	
    sscanf( ip_, "%d.%d.%d.%d", &a, &b, &c, &d );
    addr[0] = a;
    addr[1] = b;
    addr[2] = c;
    addr[3] = d;
	
    return( *(unsigned int *)addr );
}

//*****************************************************************************
char *inet_ntoa( const struct in_addr *in )
{
    char *str_ip = NULL;
    uint8_t *int_ip = NULL;
	
    str_ip = kmalloc( 16 * sizeof(char), GFP_KERNEL );
    if ( str_ip != NULL )
    {
        memset( str_ip, 0, 16 );
        int_ip = (uint8_t *)&in->s_addr;
        sprintf( str_ip, "%d.%d.%d.%d", int_ip[3], int_ip[2], int_ip[1], int_ip[0] );
    } // end if

    return( str_ip );
}



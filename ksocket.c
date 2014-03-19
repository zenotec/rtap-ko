#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <asm/processor.h>
#include <asm/uaccess.h>

#include "rtap-ko.h"
#include "ksocket.h"

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

unsigned int inet_addr( char *ip_ )
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



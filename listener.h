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
//    File: listener.h
//    Description: Implements the listener proc class. This class provides
//                 functionality for creating a list of listeners that can
//                 be set via the Linux proc filesystem.
//
//*****************************************************************************

#ifndef __LISTENER_H__
#define __LISTENER_H__

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/proc_fs.h>
#include <linux/skbuff.h>

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef uint32_t rtap_listener_id_t;

struct rtap_listener;

//*****************************************************************************
// Global variables
//*****************************************************************************

extern const struct file_operations listener_fops;

//*****************************************************************************
// Function prototypes
//*****************************************************************************

extern int listener_init( void );
extern int listener_exit( void );

extern uint16_t listener_id( struct rtap_listener* l );
extern const char* listener_ipaddr( struct rtap_listener* l );
extern uint16_t listener_port( struct rtap_listener* l );
extern struct rtap_listener* listener_findbyid( rtap_listener_id_t lid );
extern int listener_send( struct rtap_listener* l, struct sk_buff *skb );

#endif

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
//    File: device.h
//    Description: 
//
//*****************************************************************************

#ifndef __DEVICE_H__
#define __DEVICE_H__

//*****************************************************************************
// Type definitions
//*****************************************************************************

//*****************************************************************************
// Global variables
//*****************************************************************************

extern const struct file_operations rtap_device_fops;

//*****************************************************************************
// Function prototypes
//*****************************************************************************

extern int rtap_device_init( void );
extern int rtap_device_exit( void );


#endif

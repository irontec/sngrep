/**************************************************************************
 **
 ** sngrep - SIP Messages flow viewer
 **
 ** Copyright (C) 2013-2018 Ivan Alonso (Kaian)
 ** Copyright (C) 2013-2018 Irontec SL. All rights reserved.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ****************************************************************************/
/**
 * @file address.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage network addresses
 *
 * Multiple structures contain source and destination address.
 * This file contains the unification of all sngrep address containers.
 *
 */

#ifndef __SNGREP_ADDRESS_H
#define __SNGREP_ADDRESS_H

#include <netinet/in.h>
#include <stdint.h>
#include <stdbool.h>

//! Address string Length
#ifdef USE_IPV6
#ifdef INET6_ADDRSTRLEN
#define ADDRESSLEN INET6_ADDRSTRLEN
#else
#define ADDRESSLEN 46
#endif
#else
#define ADDRESSLEN INET_ADDRSTRLEN
#endif

//! Shorter declaration of address structure
typedef struct address address_t;

/**
 * @brief Network address
 */
struct address {
    //! IP address
    char ip[ADDRESSLEN];
    //! Port
    uint16_t port;
};

/**
 * @brief Check if two address are equal (including port)
 *
 * @param addr1 Address structure
 * @param addr2 Address structure
 * @return true if addresses contain the IP address, false otherwise
 */
bool
addressport_equals(address_t addr1, address_t addr2);

/**
 * @brief Check if two address are equal (ignoring port)
 *
 * @param addr1 Address structure
 * @param addr2 Address structure
 * @return true if addresses contain the same data, false otherwise
 */
bool
address_equals(address_t addr1, address_t addr2);

/**
 * @brief Check if a given IP address belongs to a local device
 *
 * @param address Address structure
 * @return true if address is local, false otherwise
 */
bool
address_is_local(address_t addr);

/**
 * @brief Convert string IP:PORT to address structure
 *
 * @param string in format IP:PORT
 * @return address structure
 */
address_t
address_from_str(const char *ipport);


#endif /* __SNGREP_ADDRESS_H */

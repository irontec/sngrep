/**************************************************************************
**
** sngrep - SIP Messages flow viewer
**
** Copyright (C) 2013-2019 Ivan Alonso (Kaian)
** Copyright (C) 2013-2019 Irontec SL. All rights reserved.
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
#ifndef SNGREP_CONFIG_H
#define SNGREP_CONFIG_H

#define PACKAGE "@PROJECT_NAME@"
#define VERSION "@PROJECT_VERSION@"

/** Defined if HEP support is enabled **/
#cmakedefine USE_HEP
/** Defined if IPv6 support is enabled **/
#cmakedefine USE_IPV6
/** Defined if TLS packet support is enabled **/
#cmakedefine WITH_SSL
/** Defined if Save to WAV support is enabled **/
#cmakedefine WITH_SND
/** Defined if RTP Playback support is enabled **/
#cmakedefine WITH_PULSE
/** Defined if G729A decode support is enabled **/
#cmakedefine WITH_G729

#endif

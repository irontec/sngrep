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
/**
 * @file flow_msg_arrow.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief
 *
 */
#ifndef __SNGREP_FLOW_MSG_ARROW_H__
#define __SNGREP_FLOW_MSG_ARROW_H__

#include <glib.h>
#include <glib-object.h>
#include "storage/message.h"
#include "tui/widgets/flow_arrow.h"

G_BEGIN_DECLS

#define SNG_TYPE_FLOW_MSG_ARROW sng_flow_msg_arrow_get_type()
G_DECLARE_FINAL_TYPE(SngFlowMsgArrow, sng_flow_msg_arrow, SNG, FLOW_MSG_ARROW, SngFlowArrow)

struct _SngFlowMsgArrow
{
    //! Parent object attributes
    SngFlowArrow parent;
    //! Item owner of this arrow
    Message *msg;
};

SngWidget *
sng_flow_msg_arrow_new(const Message *msg);

Message *
sng_flow_msg_arrow_get_message(SngFlowMsgArrow *msg_arrow);

G_END_DECLS

#endif    /* __SNGREP_FLOW_MSG_ARROW_H__ */

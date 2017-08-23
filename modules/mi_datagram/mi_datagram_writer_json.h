/*
 * $Id$
 *
 * This file is part of opensips, a free SIP server.
 *
 * opensips is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * opensips is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _MI_DATAGRAM_WRITER_JSON_H_
#define _MI_DATAGRAM_WRITER_JSON_H_

#include "datagram_fnc.h"
#include "../../mi/tree.h"

int mi_datagram_writer_json_init(unsigned int size);

int mi_datagram_json_write_tree(datagram_stream *dtgram, struct mi_root *tree);
int mi_datagram_json_flush_tree(datagram_stream *dtgram, struct mi_root *tree);

#endif /* _MI_DATAGRAM_WRITER_JSON_H_ */

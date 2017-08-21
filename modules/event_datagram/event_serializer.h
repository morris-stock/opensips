/* $Id$
 *
 * Copyright (C) 2017 BQVision
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
 *
 */

#ifndef _EV_SERIALIZER_H_
#define _EV_SERIALIZER_H_

#include "../../evi/evi_params.h"


#define DGRAM_BUFFER_SIZE   16384

extern char dgram_event_serialize_buffer[DGRAM_BUFFER_SIZE];

typedef int (serialize_f)(str *ev_name, evi_params_p ev_params);

typedef struct dgram_event_serializer_ {
    str name;               /* serializer name */
    serialize_f *serialize; /* serialize function */
} dgram_event_serializer_t;

/* datagram evetn serializer */
typedef struct dgram_event_serializers_ {
    dgram_event_serializer_t *serializer;
    struct dgram_event_serializers_ *next;
} dgram_event_serializers_t;

int register_dgram_event_serializer(dgram_event_serializer_t *serializer);

#endif // _EV_SERIALIZER_H_

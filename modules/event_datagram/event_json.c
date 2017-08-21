/* $Id$
 *
 * Copyright (C) 2017 OpenSIPS Solutions
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

#include "event_json.h"

#include "event_datagram.h"
#include "../../ut.h"

#include <string.h>

str serializer_name_json = str_init("json");

/* json */
static const str ev_json_comma			= str_init(", ");
static const str ev_json_colon			= str_init(": ");
static const str ev_json_object_start	= str_init("{");
static const str ev_json_object_stop	= str_init("}");
static const str ev_json_array_start	= str_init("[");
static const str ev_json_array_stop		= str_init("]");

static const str ev_json_escape			= str_init("\\");
static const str ev_json_squot			= str_init("\""); /* " */

static const str ev_json_name			= str_init("\"name\":");
static const str ev_json_params			= str_init("\"params\":");
static const str ev_json_att			= str_init("\"att\":");
static const str ev_json_val			= str_init("\"val\":");


#define DO_COPY_STR_LEN(buff, s, len) \
	do { \
		if ((buff) - dgram_buffer + 1 > DGRAM_BUFFER_SIZE) { \
			LM_ERR("buffer too small\n"); \
			return -1; \
		} \
		memcpy((buff), (s), (len)); \
		buff += (len); \
	} while (0)


#define DO_COPY_STR(buff, ss) DO_COPY_STR_LEN(buff, (ss).s, (ss).len)

int serialize_json(str *ev_name, evi_params_p ev_params)
{
	/*
	 * {
	 *   "name": "ev_name",
	 *   "params": [
	 *     { "att": "", "val": "" },
	 *     ...
	 *   ]
	 * }
	 */

	char *buff = dgram_buffer;

	DO_COPY_STR(buff, ev_json_object_start);

	do {
		DO_COPY_STR(buff, ev_json_name);
		DO_COPY_STR(buff, ev_json_squot);
		// no character that requires to escape
		DO_COPY_STR(buff, (*ev_name));
		DO_COPY_STR(buff, ev_json_squot);

		if (!ev_params)
			break;

		DO_COPY_STR(buff, ev_json_comma);
		DO_COPY_STR(buff, ev_json_params);
		DO_COPY_STR(buff, ev_json_array_start);
		evi_param_p node;
		for (node = ev_params->first; node; node = node->next) {
			DO_COPY_STR(buff, ev_json_object_start);

			/* optional: name */
			if (node->name.len && node->name.s) {
				DO_COPY_STR(buff, ev_json_att);
				DO_COPY_STR(buff, ev_json_squot);
				/* no escape character */
				DO_COPY_STR(buff, node->name);
				DO_COPY_STR(buff, ev_json_squot);
				DO_COPY_STR(buff, ev_json_comma);
			}

			/* value */
			DO_COPY_STR(buff, ev_json_val);
			DO_COPY_STR(buff, ev_json_squot);
			if (node->flags & EVI_STR_VAL) {
				int cur, pre;
				for (pre = 0, cur = 0; cur < node->val.s.len; ++cur) {
					switch (node->val.s.s[cur]) {
					case QUOTE_C:
					case ESC_C:
						if (cur > pre)
							DO_COPY_STR_LEN(buff, (node->val.s.s+pre), (cur-pre));

						pre = cur;

						DO_COPY_STR(buff, ev_json_escape);
						*buff = node->val.s.s[cur];
						++buff;
						++pre;
					default:
						break;
					}
				}
				if (pre < cur)
					DO_COPY_STR_LEN(buff, (node->val.s.s+pre), (cur-pre));
			} else if (node->flags & EVI_INT_VAL) {
				str s;
				s.s = int2str(node->val.n, &s.len);
				DO_COPY_STR(buff, s);
			} else {
				LM_DBG("unknown parameter type [%x]\n", node->flags);
			}
			DO_COPY_STR(buff, ev_json_squot);

			DO_COPY_STR(buff, ev_json_object_stop);

			if (node->next)
				DO_COPY_STR(buff, ev_json_comma);
		}
		DO_COPY_STR(buff, ev_json_array_stop);
	} while (0);

	DO_COPY_STR(buff, ev_json_object_stop);

	return (buff - dgram_buffer);
}

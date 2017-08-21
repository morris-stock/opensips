/*
 * Copyright (C) 2011 OpenSIPS Solutions
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

#include "event_plain.h"

#include "event_datagram.h"
#include "../../ut.h"

#include <string.h>

str serializer_name_plain = str_init("plain");

#define DO_COPY(buff, str, len) \
	do { \
		if ((buff) - dgram_buffer + 1 > DGRAM_BUFFER_SIZE) { \
			LM_ERR("buffer too small\n"); \
			return -1; \
		} \
		memcpy((buff), (str), (len)); \
		buff += (len); \
	} while (0)

int serialize_plain(str *ev_name, evi_params_p ev_params)
{
	evi_param_p node;
	int len;
	char *buff, *int_s, *p, *end, *old;
	char quote = QUOTE_C, esc = ESC_C;

	/* first is event name - cannot be larger than the buffer size */
	memcpy(dgram_buffer, ev_name->s, ev_name->len);
	dgram_buffer[ev_name->len] = PARAM_SEP;
	buff = dgram_buffer + ev_name->len + 1;

	if (!ev_params)
		goto end;

	for (node = ev_params->first; node; node = node->next) {
		/* parameter name */
		if (node->name.len && node->name.s) {
			DO_COPY(buff, node->name.s, node->name.len);
			DO_COPY(buff, ATTR_SEP_S, ATTR_SEP_LEN);
		}

		if (node->flags & EVI_STR_VAL) {
			/* it is a string value */
			if (node->val.s.len && node->val.s.s) {
				len++;
				/* check to see if enclose is needed */
				end = node->val.s.s + node->val.s.len;
				for (p = node->val.s.s; p < end; p++)
					if (*p == PARAM_SEP)
						break;
				if (p == end) {
					/* copy the whole buffer */
					DO_COPY(buff, node->val.s.s, node->val.s.len);
				} else {
					DO_COPY(buff, &quote, 1);
					old = node->val.s.s;
					/* search for '"' to escape */
					for (p = node->val.s.s; p < end; p++)
						if (*p == QUOTE_C) {
							DO_COPY(buff, old, p - old);
							DO_COPY(buff, &esc, 1);
							old = p;
						}
					/* copy the rest of the string */
					DO_COPY(buff, old, p - old);
					DO_COPY(buff, &quote, 1);
				}
			}
		} else if (node->flags & EVI_INT_VAL) {
			int_s = int2str(node->val.n, &len);
			DO_COPY(buff, int_s, len);
		} else {
			LM_DBG("unknown parameter type [%x]\n", node->flags);
		}
		*buff = PARAM_SEP;
		buff++;
	}

end:
	*buff = PARAM_SEP;
	buff++;

	return (buff - dgram_buffer);
}

#undef DO_COPY

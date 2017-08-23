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

#include "mi_datagram_writer_json.h"

static int mi_write_buffer_len = 0;

/* json */
static const str json_comma			= str_init(", ");
static const str json_colon			= str_init(": ");
static const str json_object_start	= str_init("{");
static const str json_object_stop	= str_init("}");
static const str json_array_start	= str_init("[");
static const str json_array_stop	= str_init("]");

static const str json_escape		= str_init("\\");
static const str json_squot			= str_init("\""); /* " */

static const str mi_json_code		= str_init("\"code\":");
static const str mi_json_reason		= str_init("");

static const str MI_JSON_CR				= str_init("\n");

static const str MI_JSON_KEY_NAME		= str_init("\"name\":");
static const str MI_JSON_KEY_VALUE		= str_init("\"value\":");
static const str MI_JSON_KEY_ATTRIBUTES	= str_init("\"attributes\":");
static const str MI_JSON_KEY_NODE		= str_init("\"node\":");
static const str MI_JSON_KEY_CHILDREN	= str_init("\"children\":");

static const str MI_JSON_NULL			= str_init("null");

static const str MI_JSON_COMMA			= str_init(", ");
static const str MI_JSON_COLON			= str_init(": ");

static const str MI_JSON_OBJECT_START	= str_init("{");
static const str MI_JSON_OBJECT_STOP	= str_init("}");
static const str MI_JSON_ARRAY_START	= str_init("[");
static const str MI_JSON_ARRAY_STOP		= str_init("]");

static const str MI_JSON_SQUOT			=  str_init("\""); /* " */


#define DO_JSON_COPY_STR(dtgram, s) \
	do { \
		if (dgram_json_write_str(dtgram, &s) != 0) \
			return -1; \
	} while (0)

#define DO_JSON_COPY_ESC_STR(dtgram, s) \
	do { \
		if (dgram_json_write_escape_str(dtgram, &s) != 0) \
			return -1; \
	} while (0)

static inline int dgram_json_write_str_len(datagram_stream *dtgram, const char *s, int len)
{
	if ((dtgram->current - dtgram->start + len) > dtgram->len) {
		LM_ERR("string too long");
		return -1;
	}

	memcpy(dtgram->current, s, len);
	dtgram->current += len;

	return 0;
}

static inline int dgram_json_write_str(datagram_stream *dtgram, const str *s)
{
	return dgram_json_write_str_len(dtgram, s->s, s->len);
}

static inline int dgram_json_write_escape_str_len(datagram_stream *dtgram, const char *s, int len)
{
	int cur, pre;

	for (pre = 0, cur = 0; cur < len; ++cur) {
		switch (s[cur]) {
		case '"':
		case '\\':
			if (cur > pre) {
				if (dgram_json_write_str_len(dtgram, (s+pre), (cur-pre)) != 0)
					return -1;
			}

			pre = cur;

			if (dgram_json_write_str(dtgram, &json_escape) != 0)
				return -1;

			*dtgram->current = s[cur];
			++dtgram->current;
			++pre;
		default:
			break;
		}
	}

	if (pre < cur) {
		if (dgram_json_write_str_len(dtgram, (s+pre), (cur-pre)) != 0)
			return -1;
	}

	return 0;
}

static inline int dgram_json_write_escape_str(datagram_stream *dtgram, const str *s)
{
	return dgram_json_write_escape_str_len(dtgram, s->s, s->len);
}

static int dgram_json_recur_write_tree(datagram_stream *dtgram, struct mi_node *tree, unsigned int flags);

int mi_datagram_writer_json_init(unsigned int size)
{
	mi_write_buffer_len = size;
	return 0;
}

static int dgram_json_write_node_hash(datagram_stream *dtgram, struct mi_node *node)
{
	DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
	DO_JSON_COPY_ESC_STR(dtgram, node->name);
	DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
	DO_JSON_COPY_STR(dtgram, MI_JSON_COLON);
	if (node->value.s!=NULL) {
	  DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
	  DO_JSON_COPY_ESC_STR(dtgram, node->value);
	  DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
	} else {
	  DO_JSON_COPY_STR(dtgram, MI_JSON_NULL);
	}

	node->flags |= MI_WRITTEN;
	return 0;
}

static int dgram_json_recur_write_node(datagram_stream *dtgram, struct mi_node *node, int dump_name)
{
	struct mi_attr *attr;
	int first = 1;

	/* if we only have name and value, then dump it like hash */
	if (dump_name && node->name.s && node->value.s && !node->attributes && !node->kids) {
		return dgram_json_write_node_hash(dtgram, node);
	}

	if (dump_name && node->name.s) {
		DO_JSON_COPY_STR(dtgram, json_squot);
		DO_JSON_COPY_ESC_STR(dtgram, node->name);
		DO_JSON_COPY_STR(dtgram, json_squot);
		DO_JSON_COPY_STR(dtgram, json_colon);
		DO_JSON_COPY_STR(dtgram, json_object_start);
	}

	/* value */
	if (node->value.s) {
		DO_JSON_COPY_STR(dtgram, MI_JSON_KEY_VALUE);
		DO_JSON_COPY_STR(dtgram, json_squot);
		DO_JSON_COPY_ESC_STR(dtgram, node->value);
		DO_JSON_COPY_STR(dtgram, json_squot);
		first = 0;
	}

	/* attributes */
	if (node->attributes) {
		if (!first) {
			DO_JSON_COPY_STR(dtgram, json_comma);
		}

		DO_JSON_COPY_STR(dtgram, MI_JSON_KEY_ATTRIBUTES);
		DO_JSON_COPY_STR(dtgram, MI_JSON_OBJECT_START);
		for (attr=node->attributes; attr!=NULL; attr=attr->next) {
			if (attr->name.s != NULL) {
				/* attribute name */
				DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
				DO_JSON_COPY_ESC_STR(dtgram, attr->name);
				DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
				DO_JSON_COPY_STR(dtgram, MI_JSON_COLON);

				/* attribute value */
				if (attr->value.s!=NULL) {
					DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
					DO_JSON_COPY_ESC_STR(dtgram, attr->value);
					DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
				} else {
					DO_JSON_COPY_STR(dtgram, MI_JSON_NULL);
				}
			}
			if (attr->next!=NULL) {
				DO_JSON_COPY_STR(dtgram, MI_JSON_COMMA);
			}
		}
		DO_JSON_COPY_STR(dtgram, MI_JSON_OBJECT_STOP);
		first = 0;
	}

	/* kids */
	if (node->kids) {
		if (!first) {
			DO_JSON_COPY_STR(dtgram, MI_JSON_COMMA);
		}
		DO_JSON_COPY_STR(dtgram, MI_JSON_KEY_CHILDREN);
		dgram_json_recur_write_tree(dtgram, node->kids, node->flags);
	}

	if (dump_name && node->name.s) {
		DO_JSON_COPY_STR(dtgram, MI_JSON_OBJECT_STOP);
	}

	return 0;
}

static int dgram_json_recur_write_tree(datagram_stream *dtgram, struct mi_node *tree, unsigned int flags)
{
	if (!tree)
		return -1;

	DO_JSON_COPY_STR(dtgram, MI_JSON_OBJECT_START);

	if (flags | MI_IS_ARRAY) {
		/* write name */
		DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
		if (tree->name.s) {
			DO_JSON_COPY_ESC_STR(dtgram, tree->name);
		}
		DO_JSON_COPY_STR(dtgram, MI_JSON_SQUOT);
		DO_JSON_COPY_STR(dtgram, MI_JSON_COLON);
		DO_JSON_COPY_STR(dtgram, MI_JSON_ARRAY_START);
		for (; tree; tree = tree->next) {
			// write node
			DO_JSON_COPY_STR(dtgram, MI_JSON_OBJECT_START);
			dgram_json_recur_write_node(dtgram, tree, 0);
			DO_JSON_COPY_STR(dtgram, MI_JSON_OBJECT_STOP);
			tree->flags |= MI_WRITTEN;
			if (tree->next) {
				DO_JSON_COPY_STR(dtgram, MI_JSON_COMMA);
			}
		}
		DO_JSON_COPY_STR(dtgram, json_array_stop);
	} else {
		for (; tree; tree = tree->next) {
			dgram_json_recur_write_node(dtgram, tree, 1);
			tree->flags |= MI_WRITTEN;
			if (tree->next) {
				DO_JSON_COPY_STR(dtgram, MI_JSON_COMMA);
			}
		}
	}

	DO_JSON_COPY_STR(dtgram, json_object_stop);

	return 0;
}

int mi_datagram_json_write_tree(datagram_stream *dtgram, struct mi_root *tree)
{
	/*
	 * copy&paste from mi_json module
	 */

	/* {
	 *   "domain": [
	 *     { },
	 *   ]
	 * }
	 */

	if (!tree)
		return -1;

	dtgram->current = dtgram->start;
	dtgram->len = mi_write_buffer_len;

	return dgram_json_recur_write_tree(dtgram, tree->node.kids, tree->node.flags);
}

int mi_datagram_json_flush_tree(datagram_stream *dtgram, struct mi_root *tree)
{
	return 0;
}

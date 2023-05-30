/*
 * Copyright (C) 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <proto.h>

#include "app_proto.h"
#include "assert.h"

// bytelen returns the number of bytes a cmdlen takes
static int bytelen(enum cmdlen cmdlen)
{
	int len;

	switch (cmdlen) {
	case LEN_1:
		len = 1;
		break;

	case LEN_4:
		len = 4;
		break;

	case LEN_32:
		len = 32;
		break;

	case LEN_128:
		len = 128;
		break;

	default:
		// Shouldn't happen
		assert(1 == 2);
	}

	return len;
}

// Send a firmware reply with a frame header, response code rspcode and
// following data in buf
void appreply(struct frame_header hdr, enum fwcmd rspcode, uint8_t *buf)
{
	size_t nbytes = 0;
	enum cmdlen len = 0; // length covering (rspcode + length of buf)

	switch (rspcode) {
	case FW_RSP_NAME_VERSION:
		len = LEN_32;
		break;

	case FW_RSP_LOAD_APP:
		len = LEN_4;
		break;

	case FW_RSP_LOAD_APP_DATA:
		len = LEN_4;
		break;

	case FW_RSP_LOAD_APP_DATA_READY:
		len = LEN_128;
		break;

	case FW_RSP_GET_UDI:
		len = LEN_32;
		break;

	default:
		qemu_puts("fwreply(): Unknown response code: 0x");
		qemu_puthex(rspcode);
		qemu_lf();
		return;
	}

	nbytes = bytelen(len);

	// Frame Protocol Header
	writebyte(genhdr(hdr.id, hdr.endpoint, 0x0, len));

	// FW protocol header
	writebyte(rspcode);
	nbytes--;

	write(buf, nbytes);
}

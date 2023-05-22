// Copyright (C) 2022 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

#ifndef APP_PROTO_H
#define APP_PROTO_H

#include <lib.h>
#include <proto.h>

// clang-format off
enum fwcmd {
	FW_CMD_NAME_VERSION		= 0x01,
	FW_RSP_NAME_VERSION		= 0x02,
	FW_CMD_LOAD_APP			= 0x03,
	FW_RSP_LOAD_APP			= 0x04,
	FW_CMD_LOAD_APP_DATA		= 0x05,
	FW_RSP_LOAD_APP_DATA		= 0x06,
	FW_RSP_LOAD_APP_DATA_READY	= 0x07,
	FW_CMD_GET_UDI			= 0x08,
	FW_RSP_GET_UDI			= 0x09,
	FW_CMD_MAX                      = 0x0a,
};
// clang-format on

void appreply_nok(struct frame_header hdr);
void appreply(struct frame_header hdr, enum fwcmd rspcode, uint8_t *buf);

#endif

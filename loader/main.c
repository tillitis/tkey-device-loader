/*
 * Copyright (C) 2022, 2023 - Tillitis AB
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <lib.h>
#include <proto.h>
#include <tk1_mem.h>

#include "app_proto.h"
#include "assert.h"

// clang-format off
// clang-format on

// Context for the loading of a TKey program
struct context {
	uint32_t left;	    // Bytes left to receive
	uint8_t digest[32]; // Program digest
	uint8_t *loadaddr;  // Where we are currently loading a TKey program
	uint32_t app_size;  // Size of app to load
};

enum state {
	FW_STATE_INITIAL,
	FW_STATE_LOADING,
	FW_STATE_RUN,
	FW_STATE_FAIL,
	FW_STATE_MAX,
};

static void print_digest(uint8_t *md);
static enum state initial_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx);
static enum state loading_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx);
static void run(const struct context *ctx);

static void print_digest(uint8_t *md)
{
	qemu_puts("The app digest:\n");
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 8; i++) {
			qemu_puthex(md[i + 8 * j]);
		}
		qemu_lf();
	}
	qemu_lf();
}

static enum state initial_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx)
{
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};

	switch (cmd[0]) {
	case FW_CMD_NAME_VERSION:
		qemu_puts("cmd: name-version\n");
		if (hdr->len != 1) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		// copy_name(rsp, CMDLEN_MAXBYTES, *name0);
		// copy_name(&rsp[4], CMDLEN_MAXBYTES - 4, *name1);
		// wordcpy_s(&rsp[8], CMDLEN_MAXBYTES / 4 - 2, (void *)ver, 1);

		appreply(*hdr, FW_RSP_NAME_VERSION, rsp);
		// still initial state
		break;

	case FW_CMD_LOAD_APP: {
		uint32_t local_app_size;

		qemu_puts("cmd: load-app(size, uss)\n");
		if (hdr->len != 128) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		// cmd[1..4] contains the size.
		local_app_size =
		    cmd[1] + (cmd[2] << 8) + (cmd[3] << 16) + (cmd[4] << 24);

		qemu_puts("app size: ");
		qemu_putinthex(local_app_size);
		qemu_lf();

		if (local_app_size == 0 || local_app_size > TK1_APP_MAX_SIZE) {
			rsp[0] = STATUS_BAD;
			appreply(*hdr, FW_RSP_LOAD_APP, rsp);
			// still initial state
			break;
		}

		ctx->app_size = local_app_size;

		// Do we have a USS at all?
		/* if (cmd[5] != 0) { */
		/* 	// Yes */
		/* 	ctx->use_uss = TRUE; */
		/* 	memcpy_s(ctx->uss, 32, &cmd[6], 32); */
		/* } else { */
		/* 	ctx->use_uss = FALSE; */
		/* } */

		rsp[0] = STATUS_OK;
		appreply(*hdr, FW_RSP_LOAD_APP, rsp);

		ctx->left = ctx->app_size;

		state = FW_STATE_LOADING;
		break;
	}

	default:
		qemu_puts("Got unknown firmware cmd: 0x");
		qemu_puthex(cmd[0]);
		qemu_lf();
		state = FW_STATE_FAIL;
		break;
	}

	return state;
}

static enum state loading_commands(const struct frame_header *hdr,
				   const uint8_t *cmd, enum state state,
				   struct context *ctx)
{
	uint8_t rsp[CMDLEN_MAXBYTES] = {0};
	uint32_t nbytes = 0;

	switch (cmd[0]) {
	case FW_CMD_LOAD_APP_DATA:
		qemu_puts("cmd: load-app-data\n");
		if (hdr->len != 128) {
			// Bad length
			state = FW_STATE_FAIL;
			break;
		}

		if (ctx->left > (128 - 1)) {
			nbytes = 128 - 1;
		} else {
			nbytes = ctx->left;
		}
		memcpy(ctx->loadaddr, cmd + 1, nbytes);
		ctx->loadaddr += nbytes;
		ctx->left -= nbytes;

		if (ctx->left == 0) {
			blake2s_ctx b2s_ctx = {0};
			int blake2err = 0;

			qemu_puts("Fully loaded ");
			qemu_putinthex(ctx->app_size);
			qemu_lf();

			// Compute Blake2S digest of the app,
			// storing it for FW_STATE_RUN
			blake2err = blake2s(&ctx->digest, 32, NULL, 0,
					    (const void *)TK1_RAM_BASE,
					    ctx->app_size, &b2s_ctx);
			assert(blake2err == 0);
			print_digest(ctx->digest);

			// And return the digest in final
			// response
			rsp[0] = STATUS_OK;
			memcpy(&rsp[1], &ctx->digest, 32);
			appreply(*hdr, FW_RSP_LOAD_APP_DATA_READY, rsp);

			state = FW_STATE_RUN;
			break;
		}

		rsp[0] = STATUS_OK;
		appreply(*hdr, FW_RSP_LOAD_APP_DATA, rsp);
		// still loading state
		break;

	default:
		qemu_puts("Got unknown firmware cmd: 0x");
		qemu_puthex(cmd[0]);
		qemu_lf();
		state = FW_STATE_FAIL;
		break;
	}

	return state;
}

static void run(const struct context *ctx)
{
	//*app_addr = TK1_RAM_BASE;

	qemu_puts("Jumping to second app\n");
	/* qemu_putinthex(*app_addr); */
	/* qemu_lf(); */

	// Jump to app - doesn't return
	// clang-format off
#ifndef S_SPLINT_S
	asm volatile(
		// Get value at TK1_MMIO_TK1_APP_ADDR
		"lui a0,0xff000;"
		"lw a0,0x030(a0);"
		// Jump to it
		"jalr x0,0(a0);"
		::: "memory");
#endif
	// clang-format on

	__builtin_unreachable();
}

int readcommand(struct frame_header *hdr, uint8_t *cmd, int state)
{
	uint8_t in = 0;

	// set_led((state == FW_STATE_LOADING) ? LED_BLACK : LED_WHITE);
	in = readbyte();

	if (parseframe(in, hdr) == -1) {
		qemu_puts("Couldn't parse header\n");
		return -1;
	}

	(void)memset(cmd, 0, CMDLEN_MAXBYTES);
	// Now we know the size of the cmd frame, read it all
	read(cmd, hdr->len);

	// Is it for us?
	if (hdr->endpoint != DST_FW) {
		qemu_puts("Message not meant for us\n");
		return -1;
	}

	return 0;
}

int main()
{
	struct context ctx = {0};
	struct frame_header hdr = {0};
	uint8_t cmd[CMDLEN_MAXBYTES] = {0};
	enum state state = FW_STATE_INITIAL;

	ctx.loadaddr = (uint8_t *)TK1_RAM_BASE;

	for (;;) {
		switch (state) {
		case FW_STATE_INITIAL:
			if (readcommand(&hdr, cmd, state) == -1) {
				state = FW_STATE_FAIL;
				break;
			}

			state = initial_commands(&hdr, cmd, state, &ctx);
			break;

		case FW_STATE_LOADING:
			if (readcommand(&hdr, cmd, state) == -1) {
				state = FW_STATE_FAIL;
				break;
			}

			state = loading_commands(&hdr, cmd, state, &ctx);
			break;

		case FW_STATE_RUN:
			run(&ctx);
			break; // This is never reached!

		case FW_STATE_FAIL:
			// fallthrough
		default:
			qemu_puts("firmware state 0x");
			qemu_puthex(state);
			qemu_lf();
			// Force illegal instruction to halt CPU
			asm volatile("unimp");
			break; // Not reached
		}
	}

	return (int)0xcafebabe;
}

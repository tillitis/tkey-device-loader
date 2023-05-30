OBJCOPY ?= llvm-objcopy

P := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
LIBDIR ?= $(P)/../tkey-libs

CC = clang

INCLUDE=$(LIBDIR)/include

# If you want libcommon's qemu_puts() et cetera to output something on our QEMU
# debug port, remove -DNODEBUG below
CFLAGS = -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 -mcmodel=medany \
   -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf \
   -fno-builtin-putchar -nostdlib -mno-relax -flto -g \
   -Wall -Werror=implicit-function-declaration \
   -I $(INCLUDE) -I $(LIBDIR)  \
   -DNODEBUG

AS = clang
ASFLAGS = -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 -mcmodel=medany -mno-relax

LDFLAGS=-T $(LIBDIR)/app.lds -L $(LIBDIR)/libcommon/ -lcommon -L $(LIBDIR)/libcrt0/ -lcrt0

RM=/bin/rm


.PHONY: all
all: loader/app.bin tkey-loader

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

OBJS=loader/main.o loader/assert.o loader/app_proto.o
loader/app.elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -L $(LIBDIR)/monocypher -lmonocypher -I $(LIBDIR) -o $@
$(OBJS): $(INCLUDE)/tk1_mem.h #loader/app_proto.h

# .PHONY to let go build handle deps and rebuilds
.PHONY: 
tkey-loader: loader/app.bin
	cp -af loader/app.bin go-loader/loader.bin
	go build -o tkey-loader ./go-loader


.PHONY: clean
clean:
	$(RM) -f tkey-loader loader/app.bin loader/app.elf $(OBJS)

# Uses ../.clang-format
FMTFILES=loader/*.[ch]

.PHONY: fmt
fmt:
	clang-format --dry-run --ferror-limit=0 $(FMTFILES)
	clang-format --verbose -i $(FMTFILES)
.PHONY: checkfmt
checkfmt:
	clang-format --dry-run --ferror-limit=0 --Werror $(FMTFILES)

.PHONY: podman
podman:
	podman run --rm --mount type=bind,source=$(CURDIR),target=/src --mount type=bind,source=$(LIBDIR),target=/tkey-libs -w /src -it ghcr.io/tillitis/tkey-builder:2 make -j

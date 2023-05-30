# TKey loader app

Warning: hackathon quality!

The [Tillitis](https://tillitis.se/) TKey unconditionally measures
device apps to create a unique identity. This makes it hard to upgrade
device apps without changing the identity. A solution is to combine
measured boot and verified boot in a loader app.

The loader device app is first loaded into the TKey together with a
public key, then mimics the firmware protocol and loads another device
app, verifies its signature against the public key, and then starts
it. The CDI remains the same.

**Warning**: This repo contains a first PoC cobbled together during a
Tillitis hackathon. It doesn't do any verification and it doesn't even
put the second device app in the right place. This is just a framework
to start with.

Example Go code is in `go-loader` to show how you can use the loader
in your own program, but basically you just call `LoadApp()` twice:

```go
tk := tkeyclient.New()
...
appBin, err := os.ReadFile(fileName)
...
appBin2, err := os.ReadFile(fileName2)
...
err = tk.LoadApp(appBin, secret)
err = tk.LoadApp(appBin2, secret)
```

To build and run `go-loader`:

```
$ cd go-builder
$ go build

```

To run, for example with the blink device app in `~/tillitis/git/tillitis-key1-apps/apps/blink/app.bin`:

```
$  ./go-loader --port /dev/ttyACM0 ../loader/app.bin ~/tillitis/git/tillitis-key1-apps/apps/blink/app.bin
```

## Building

You have two options for build tools: either use our OCI image
`ghcr.io/tillitis/tkey-builder` or native tools.

In either case, you need the device libraries in a directory next to
this one. The device libraries are available in:

https://github.com/tillitis/tkey-libs

Clone and build the device libraries first. You will most likely want
to specify a release with something like `-b v0.0.1`:

```
$ git clone -b v0.0.1 --depth 1 https://github.com/tillitis/tkey-libs
$ cd tkey-libs
$ make
```

### Building with Podman

We provide an OCI image with all the tools needed to build both `tkey-libs`
and this application. If you have `make` and Podman installed you can use it
for the `tkey-libs` directory and then this directory:

```
make podman
```

and everything should be built. This assumes a working rootless
podman. On Ubuntu 22.10, running

```
apt install podman rootlesskit slirp4netns
```

should be enough to get you a working Podman setup.

### Building with native tools

To build with native tools you need at least the `clang`, `llvm`,
`lld`, packages installed. Version 15 or later of LLVM/Clang is for
support of our architecture (RV32\_Zmmul). Ubuntu 22.10 (Kinetic) is
known to have this. Please see
[toolchain_setup.md](https://github.com/tillitis/tillitis-key1/blob/main/doc/toolchain_setup.md)
(in the tillitis-key1 repository) for detailed information on the
currently supported build and development environment.

Build everything:

```
$ make
```

If you cloned `tkey-libs` to somewhere else then the default, set
`LIBDIR` to the path of the directory.

If your available `objcopy` is anything other than the default
`llvm-objcopy`, then define `OBJCOPY` to whatever they're called on
your system.

## Licenses and SPDX tags

Unless otherwise noted, the project sources are licensed under the
terms and conditions of the "GNU General Public License v2.0 only":

> Copyright Tillitis AB.
>
> These programs are free software: you can redistribute it and/or
> modify it under the terms of the GNU General Public License as
> published by the Free Software Foundation, version 2 only.
>
> These programs are distributed in the hope that they will be useful,
> but WITHOUT ANY WARRANTY; without even the implied warranty of
> MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
> General Public License for more details.

> You should have received a copy of the GNU General Public License
> along with this program. If not, see:
>
> https://www.gnu.org/licenses

See [LICENSE](LICENSE) for the full GPLv2-only license text.

External source code we have imported are isolated in their own
directories. They may be released under other licenses. This is noted
with a similar `LICENSE` file in every directory containing imported
sources.

The project uses single-line references to Unique License Identifiers
as defined by the Linux Foundation's [SPDX project](https://spdx.org/)
on its own source files, but not necessarily imported files. The line
in each individual source file identifies the license applicable to
that file.

The current set of valid, predefined SPDX identifiers can be found on
the SPDX License List at:

https://spdx.org/licenses/

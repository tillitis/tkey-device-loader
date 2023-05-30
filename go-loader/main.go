// Copyright (C) 2022, 2023 - Tillitis AB
// SPDX-License-Identifier: GPL-2.0-only

package main

import (
	"bytes"
	_ "embed"
	"fmt"
	"log"
	"os"
	"os/signal"
	"strings"
	"syscall"

	"github.com/spf13/pflag"
	"github.com/tillitis/tkeyclient"
)

// nolint:typecheck // Avoid lint error when the embedding file is missing.
// Makefile copies the built app here ./app.bin
//
//go:embed loader.bin
var loaderBinary []byte

// Use when printing err/diag msgs
var le = log.New(os.Stderr, "", 0)

func main() {
	var fileName, devPath string
	var speed int
	var verbose, helpOnly bool
	pflag.CommandLine.SetOutput(os.Stderr)
	pflag.CommandLine.SortFlags = false
	pflag.StringVar(&devPath, "port", "",
		"Set serial port device `PATH`. If this is not passed, auto-detection will be attempted.")
	pflag.IntVar(&speed, "speed", tkeyclient.SerialSpeed,
		"Set serial port speed in `BPS` (bits per second).")
	pflag.BoolVar(&verbose, "verbose", false, "Enable verbose output.")
	pflag.BoolVar(&helpOnly, "help", false, "Output this help.")
	pflag.Usage = func() {
		desc := fmt.Sprintf(`Usage: %[1]s [flags...] FILE

%[1]s loads an application binary from FILE onto Tillitis TKey
and starts it.

Exit status code is 0 if the app is both successfully loaded and started. Exit
code is non-zero if anything goes wrong, for example if TKey is already
running some app.`, os.Args[0])
		le.Printf("%s\n\n%s", desc,
			pflag.CommandLine.FlagUsagesWrapped(86))
	}
	pflag.Parse()

	if pflag.NArg() > 0 {
		if pflag.NArg() > 1 {
			le.Printf("Unexpected argument: %s\n\n", strings.Join(pflag.Args()[1:], " "))
			pflag.Usage()
			os.Exit(2)
		}
		fileName = pflag.Args()[0]
	}

	if helpOnly {
		pflag.Usage()
		os.Exit(0)
	}

	if fileName == "" {
		le.Printf("Please pass an app binary FILE.\n\n")
		pflag.Usage()
		os.Exit(2)
	}

	if !verbose {
		tkeyclient.SilenceLogging()
	}

	appBin, err := os.ReadFile(fileName)
	if err != nil {
		le.Printf("Failed to read file: %v\n", err)
		os.Exit(1)
	}
	if bytes.HasPrefix(appBin, []byte("\x7fELF")) {
		le.Printf("%s looks like an ELF executable, but a raw binary is expected.\n", fileName)
		os.Exit(1)
	}

	if devPath == "" {
		le.Printf("Please provide a --port.\n\n")
		os.Exit(1)
	}

	tk := tkeyclient.New()
	le.Printf("Connecting to device on serial port %s ...\n", devPath)
	if err = tk.Connect(devPath, tkeyclient.WithSpeed(speed)); err != nil {
		le.Printf("Could not open %s: %v\n", devPath, err)
		os.Exit(1)
	}
	exit := func(code int) {
		if err = tk.Close(); err != nil {
			le.Printf("Close: %v\n", err)
		}
		os.Exit(code)
	}
	handleSignals(func() { exit(1) }, os.Interrupt, syscall.SIGTERM)

	nameVer, err := tk.GetNameVersion()
	if err != nil {
		le.Printf("GetNameVersion failed: %v\n", err)
		le.Printf("If the serial port is correct, then the TKey might not be in firmware-\n" +
			"mode, and have an app running already. Please unplug and plug it in again.\n")
		exit(1)
	}
	le.Printf("Firmware name0:'%s' name1:'%s' version:%d\n",
		nameVer.Name0, nameVer.Name1, nameVer.Version)

	udi, err := tk.GetUDI()
	if err != nil {
		le.Printf("GetUDI failed: %v\n", err)
		exit(1)
	}

	fmt.Printf("UDI: %v\n", udi)

	var secret []byte

	le.Printf("Loading app from %v onto device\n", fileName)

	err = tk.LoadApp(loaderBinary, secret)
	if err != nil {
		le.Printf("LoadAppFromFile failed: %v\n", err)
		exit(1)
	}

	le.Printf("Loading app from %v onto device\n", fileName)

	err = tk.LoadApp(appBin, secret)
	if err != nil {
		le.Printf("LoadAppFromFile failed: %v\n", err)
		exit(1)
	}

	exit(0)
}

func handleSignals(action func(), sig ...os.Signal) {
	ch := make(chan os.Signal, 1)
	signal.Notify(ch, sig...)
	go func() {
		for {
			<-ch
			action()
		}
	}()
}

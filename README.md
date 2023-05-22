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

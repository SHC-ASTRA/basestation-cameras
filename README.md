# Basestation Cameras (Qt6 + GStreamer)

Multi-RTSP grid viewer using Qt6 and GStreamer, controlled via IPC (Unix domain socket).

## Dev shell

```
nix develop
```

## Build

```
cmake -S . -B build -G Ninja
cmake --build build -j
```

## Run

```
./build/basestation-cameras
```

## IPC protocol

- Socket path: `/tmp/basestation-cameras-ipc` (QLocalServer)
- Transport: newline-delimited JSON commands
- Format:

```
{"cmd":"add_stream","payload":{"id":"cam1","url":"rtsp://example/stream"}}
{"cmd":"remove_stream","payload":{"id":"cam1"}}
{"cmd":"grid","payload":{"rows":2,"cols":3}}
```

### Send command example

```
printf '%s\n' '{"cmd":"grid","payload":{"rows":2,"cols":2}}' | socat - UNIX-CONNECT:/tmp/basestation-cameras-ipc
```

### Notes

- The first frame decode can take a moment depending on RTSP server.
- To reduce latency, consider switching to `rtspsrc ! rtph264depay ! avdec_h264 ! videoconvert ! appsink` with appropriate properties; current pipeline uses `uridecodebin` for simplicity.


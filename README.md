# Basestation Cameras

Multi-RTSP grid viewer using Qt6 and GStreamer, controlled via IPC

## Dev shell

```
nix develop
```

## Build

```
nix build
```

## Run

```
nix run
```

## IPC protocol

- Socket path: `/tmp/basestation-cameras-ipc`
- Transport: newline-delimited JSON commands
- Format:

```
{"cmd":"add_stream","payload":{"id":"cam1","url":"rtsp://example/stream"}}
{"cmd":"remove_stream","payload":{"id":"cam1"}}
{"cmd":"grid","payload":{"rows":2,"cols":3}}
```

### Send command example

```
printf '{"cmd":"add_stream","payload":{"id":%s,"url":%s}}\n' $id $url | socat - UNIX-CONNECT:/tmp/basestation-cameras-ipc
printf '{"cmd":"remove_stream","payload":{"id":%s}}\n' $id | socat - UNIX-CONNECT:/tmp/basestation-cameras-ipc
printf '{"cmd":"grid","payload":{"rows":%s,"cols":%s}}\n' $rows $cols | socat - UNIX-CONNECT:/tmp/basestation-cameras-ipc
```


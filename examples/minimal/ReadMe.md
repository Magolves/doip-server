# Minimal DoIP Client/Server Example

Server listens on TCP port 13400, client connects to server and sends the following UDS message sequence

1. Open a diag session `10.02`
2. Read VIN via RDBI 0xF190 `22.F1.90`
3. Close diag session / switch back to default session `10.01`

## Build the example

```bash
$ cmake . -Bbuild
$ cd build
$ make -j
$ cd examples/minimal
```

## Start the server

```bash
$ ./MinimalDoIPServer
...
[13:42:49.197] [tcp ] [info] TCP transport ready and listening on port 13400
...

```bash
$ ./MinimalDoIPClient
...
# Open diag session
[2026-01-08 13:41:52.039] [doip-client] [info] TX: V03|Diag E000 -> 28: 10.02
[2026-01-08 13:41:52.039] [doip-client] [info] RX: V03|DiagnosticMessageAck (0x8002)|L5| Payload: E0.28.00.28.00
[2026-01-08 13:41:52.099] [doip-client] [info] RX: V03|Diag 28 -> E000: 50.02.03.E8.13.88
# Read VIN
[2026-01-08 13:41:52.099] [doip-client] [info] TX: V03|Diag E000 -> 28: 22.F1.90
[2026-01-08 13:41:52.099] [doip-client] [info] RX: V03|DiagnosticMessageAck (0x8002)|L5| Payload: E0.28.00.28.00
[2026-01-08 13:41:52.159] [doip-client] [info] RX: V03|Diag 28 -> E000: 62.F1.90.57.56.57.5A.5A.5A.31.4A.5A.34.57.30.31.32.33.34.35
# Close diag session
[2026-01-08 13:41:52.159] [doip-client] [info] TX: V03|Diag E000 -> 28: 10.01
[2026-01-08 13:41:52.159] [doip-client] [info] RX: V03|DiagnosticMessageAck (0x8002)|L5| Payload: E0.00.00.28.00
[2026-01-08 13:41:52.219] [doip-client] [info] RX: V03|Diag 28 -> E000: 50.01.03.E8.13.88
```
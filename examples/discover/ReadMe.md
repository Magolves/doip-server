# Example illustrating the "discover" mechanism.

- Server sends vehicle announcements/responses (payload type `0x0004`); then listens for vehicle requests
- Client listens for vehicle announcements; then send a vehicle request (payload type `0x0001`)
- When the client received a vehicle identification response, it connects to the TCP endpoint
- Client sends a routing activation request and expects a response
- Client sends a diagnostic UDS message (RDBI 0x22 with DID 0xF190): `22.F1.90`
- Server responds with Diag Ack, then sends the response which should contain the VIN: `62.F1.90.57.56.57.5A.5A.5A.31.4A.5A.34.57.30.31.32.33.34.35`


## Build the example

```bash
$ cmake . -Bbuild
$ cd build
$ make -j
$ cd examples/discover
```

## Start the server

```bash
$ ./DoIPServer
# Send 3 vehicle announcements with 500ms interval
...
[13:18:03.469] [server] [info] TX V03|VehicleIdentificationResponse (0x0004)|L33| Payload: 57.56.57.5A.5A.5A.31.4A.5A.33.57.33.38.36.37.35.32.00.28.00.00.00.65.43.21.00.00.00.12.34.56.00.00
[13:18:03.969] [server] [info] TX V03|VehicleIdentificationResponse (0x0004)|L33| Payload: 57.56.57.5A.5A.5A.31.4A.5A.33.57.33.38.36.37.35.32.00.28.00.00.00.65.43.21.00.00.00.12.34.56.00.00
[13:18:04.469] [server] [info] TX V03|VehicleIdentificationResponse (0x0004)|L33| Payload: 57.56.57.5A.5A.5A.31.4A.5A.33.57.33.38.36.37.35.32.00.28.00.00.00.65.43.21.00.00.00.12.34.56.00.00
```

## Run the client

The client (please note the `--loopback` argument) then tries to discover the DOIP server - either by receiving an announcement or sending actively a vehicle

```bash
$ python3 discover-client.py --loopback
...
No announcement received, sending identification request...
Sending vehicle identification request...
Request sent to 127.0.0.1:13400
Received response from 127.0.0.1:13400
  VIN: WVWZZZ1JZ3W386752
  Logical Address: 0x0028
  EID: 000000654321
  GID: 000000123456
  Further Action: 0x00
==================================================
Vehicle discovered successfully!
==================================================
...
```

Then the client connects to the TCP socket using the IP address of the server response:

```bash
Connecting to 127.0.0.1:13400...
TCP connection established
```

Now the client sends a routing activation request...

```bash
Sending routing activation request...
  Response Code: 0x10
  Tester Address: 0x0E80
  Entity Address: 0x0028
  Status: Successfully activated
```

... and finally reads the VIN via UDS RDBI

```bash
Sending UDS RDBI request for DID 0xF190...
  Source: 0x0028, Target: 0x0E80
  UDS Response: 62f1905756575a5a5a314a5a3457303132333435
  Service: Positive Response (0x62)
  DID: 0xF190
  Data: 5756575a5a5a314a5a3457303132333435
  Data (ASCII): WVWZZZ1JZ4W012345

```

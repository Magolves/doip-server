# DoIP Server: Logger Factory & Daemon Mode - Complete Guide

## Overview

This guide demonstrates a professional C++17 implementation for dynamic logger switching between console and syslog, integrated with proper daemon mode support.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         main()                               │
│  1. Parse args                                              │
│  2. Initialize logger (console)                             │
│  3. Daemonize (if --daemon)                                 │
│  4. Switch logger to syslog (if daemon)                     │
│  5. Construct DoIPServer                                    │
│  6. Run server                                              │
└─────────────────────────────────────────────────────────────┘
         │
         ├─ Console Mode              ├─ Daemon Mode
         │                            │
         v                            v
┌──────────────────┐         ┌──────────────────┐
│ LoggerFactory    │         │ LoggerFactory    │
│ Mode: Console    │         │ Mode: Syslog     │
│ Output: stdout   │         │ Output: syslogd  │
│ Colors: Yes      │         │ Colors: No       │
└──────────────────┘         └──────────────────┘
```

## Key Features

### 1. Dynamic Logger Switching
- **Console Mode**: Colored output to stdout/stderr
- **Syslog Mode**: System log daemon integration
- **Runtime Switching**: Change logging mode without recompilation

### 2. Thread-Safe Logger Factory
- Singleton pattern with lazy initialization
- Mutex-protected logger registry
- Avoids static initialization order issues

### 3. Proper Daemon Implementation
- Double-fork technique
- Session leader establishment
- File descriptor cleanup
- PID file management

## Implementation

### File Structure

```
doip_server/
├── include/
│   ├── Logger.h              # Enhanced logger factory (NEW)
│   ├── daemon_utils.h        # Daemon utilities (NEW)
│   ├── DoIPServer.h          # Modified (remove daemonize from config)
│   └── ...
├── src/
│   ├── main.cpp              # Updated with logger factory
│   ├── daemon_utils.cpp      # Daemon implementation (NEW)
│   ├── DoIPServer.cpp        # Modified (remove ctor daemonization)
│   └── ...
└── CMakeLists.txt
```

### Changes Required

#### 1. Update Logger.h

Replace your current `Logger` class with the `LoggerFactory` implementation.

**Key Changes:**
- Add `LoggerMode` enum (Console, Syslog, File)
- Add `LoggerConfig` struct
- Add `initialize()` and `switchToSyslog()` methods
- Thread-safe logger registry
- Backward compatible (keep `Logger` as alias to `LoggerFactory`)

**New API:**
```cpp
// Initialize logger (call early in main)
LoggerFactory::initialize(LoggerConfig{
    .mode = LoggerMode::Console,
    .level = spdlog::level::info
});

// After daemonization
LoggerFactory::switchToSyslog("doipd", LOG_DAEMON);

// Existing code continues to work
LOG_DOIP_INFO("Server started");  // No changes needed!
```

#### 2. Update DoIPServer.h

**Remove from ServerConfig:**
```cpp
struct ServerConfig {
    DoIpEid eid = DoIpEid::Zero;
    DoIpGid gid = DoIpGid::Zero;
    DoIpVin vin = DoIpVin::Zero;
    DoIPAddress logicalAddress = DoIPAddress(0x0028);
    bool loopback = false;
    // REMOVED: bool daemonize = false;  ← Delete this line
    int announceCount = 3;
    unsigned int announceInterval = 500;
};
```

**Remove from DoIPServer class:**
```cpp
class DoIPServer {
    // ...
private:
    // REMOVED: void daemonize();  ← Delete this method
};
```

#### 3. Update DoIPServer.cpp

**Remove daemonization from constructor:**
```cpp
// OLD (incorrect):
DoIPServer::DoIPServer(const ServerConfig &config)
    : m_config(config) {
    m_receiveBuf.reserve(DOIP_MAXIMUM_MTU);
    setLoopbackMode(m_config.loopback);
    
    if (m_config.daemonize) {
        daemonize();  // ❌ Remove this
    }
}

// NEW (correct):
DoIPServer::DoIPServer(const ServerConfig &config)
    : m_config(config) {
    m_receiveBuf.reserve(DOIP_MAXIMUM_MTU);
    setLoopbackMode(m_config.loopback);
    // No daemonization here!
}
```

**Delete the daemonize() member function:**
```cpp
// Delete entire function:
void DoIPServer::daemonize() { ... }  // ❌ Delete
```

#### 4. Create daemon_utils.h and daemon_utils.cpp

Add these new files to your project (see attached files).

#### 5. Update main.cpp

See `main_complete_example.cpp` for full implementation.

**Critical sequence:**
```cpp
int main(int argc, char* argv[]) {
    // 1. Parse arguments
    bool daemon_mode = /* ... */;
    
    // 2. Initialize logger (CONSOLE mode)
    LoggerFactory::initialize(LoggerConfig{
        .mode = LoggerMode::Console,
        .level = spdlog::level::info
    });
    
    LOG_DOIP_INFO("Starting server...");
    
    // 3. Daemonize FIRST (if requested)
    if (daemon_mode) {
        if (!daemon::daemonize("/var/run/doipd.pid")) {
            return 1;
        }
        
        // 4. Switch logger to SYSLOG (after daemonization)
        LoggerFactory::switchToSyslog("doipd", LOG_DAEMON);
        LOG_DOIP_INFO("Daemon started");  // Goes to syslog now
    }
    
    // 5. Construct server
    DoIPServer server(config);
    
    // 6. Setup sockets
    server.setupTcpSocket();
    server.setupUdpSocket();
    
    // 7. Run
    while (server.isRunning()) {
        sleep(1);
    }
    
    return 0;
}
```

## Usage Examples

### Running in Foreground (Console Logging)

```bash
# Basic foreground mode
./doipd

# With debug logging
./doipd --verbose

# With trace logging (very verbose)
./doipd --trace

# Output:
[12:34:56.789] [doip] [info] DoIP Server starting...
[12:34:56.790] [doip] [info] Running in foreground mode (console logging)
[12:34:56.791] [tcp ] [info] TCP socket listening on port 13400
[12:34:56.792] [udp ] [info] UDP socket bound on port 13400
==================================================
  DoIP Server Ready
  VIN: WAUZZZ8V9KA123456
  Logical Address: 0x0028
  TCP Port: 13400
  UDP Port: 13400
  Mode: Foreground (console)
==================================================
```

### Running as Daemon (Syslog Logging)

```bash
# Start as daemon
./doipd --daemon

# Check it's running
ps aux | grep doipd
cat /var/run/doipd.pid

# View syslog entries
journalctl -t doipd -f

# Or traditional syslog
tail -f /var/log/syslog | grep doipd

# Stop daemon
kill $(cat /var/run/doipd.pid)

# Or
systemctl stop doipd
```

**Syslog output:**
```
Dec 20 12:34:56 hostname doipd[12345]: [doip] Daemon started successfully (PID: 12345)
Dec 20 12:34:56 hostname doipd[12345]: [tcp ] TCP socket listening on port 13400
Dec 20 12:34:56 hostname doipd[12345]: [udp ] UDP socket bound on port 13400
Dec 20 12:34:56 hostname doipd[12345]: [doip] DoIP Server Ready
```

### Advanced Options

```bash
# Custom VIN
./doipd --daemon --vin WBADT43452G123456

# Custom logical address
./doipd --daemon --address 0x1234

# Custom PID file location
./doipd --daemon --pidfile /tmp/doipd.pid

# Debug mode as daemon
./doipd --daemon --verbose

# Trace logging (very detailed)
./doipd --daemon --trace
```

## systemd Integration (Recommended for Production)

### Create systemd service file

```ini
# /etc/systemd/system/doipd.service
[Unit]
Description=DoIP Server (ISO 13400)
After=network-online.target
Wants=network-online.target
Documentation=man:doipd(8)

[Service]
Type=simple
ExecStart=/usr/local/bin/doipd
Restart=on-failure
RestartSec=5s
User=doip
Group=doip

# Logging to systemd journal (no need for syslog)
StandardOutput=journal
StandardError=journal
SyslogIdentifier=doipd

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/doip

# Resource limits
LimitNOFILE=65536
MemoryMax=512M

[Install]
WantedBy=multi-user.target
```

### Modified main.cpp for systemd

When using systemd, you don't need manual daemonization:

```cpp
int main(int argc, char* argv[]) {
    // Parse arguments
    bool use_syslog = /* check for --syslog or detect systemd */;
    
    // Initialize logger
    if (use_syslog || getenv("INVOCATION_ID")) {  // systemd sets INVOCATION_ID
        // Running under systemd - use syslog
        LoggerFactory::initialize(LoggerConfig{
            .mode = LoggerMode::Syslog,
            .syslog_ident = "doipd"
        });
    } else {
        // Running interactively - use console
        LoggerFactory::initialize(LoggerConfig{
            .mode = LoggerMode::Console
        });
    }
    
    // No manual daemonization - systemd handles it
    
    // Rest of server initialization...
}
```

### systemd commands

```bash
# Enable service
sudo systemctl enable doipd

# Start service
sudo systemctl start doipd

# Check status
sudo systemctl status doipd

# View logs
sudo journalctl -u doipd -f

# View logs since boot
sudo journalctl -u doipd -b

# Stop service
sudo systemctl stop doipd

# Restart service
sudo systemctl restart doipd

# Reload configuration (send SIGHUP)
sudo systemctl reload doipd
```

## Debugging

### Console Mode Debugging

```bash
# Run with strace
strace -f -e trace=network,signal ./doipd

# Run with valgrind
valgrind --leak-check=full ./doipd

# Run with gdb
gdb ./doipd
(gdb) run
(gdb) bt  # if it crashes
```

### Daemon Mode Debugging

```bash
# Increase log level
./doipd --daemon --trace

# Watch syslog in real-time
journalctl -t doipd -f

# Check if daemon is really detached
ps aux | grep doipd
# Should show no controlling terminal (? in TTY column)

# Check file descriptors
lsof -p $(cat /var/run/doipd.pid)
# stdin/stdout/stderr should point to /dev/null

# Check process tree
pstree -p $(cat /var/run/doipd.pid)
```

## Logger Factory API Reference

### Initialization

```cpp
// Initialize with default config (console mode)
LoggerFactory::initialize();

// Initialize with custom config
LoggerFactory::initialize(LoggerConfig{
    .mode = LoggerMode::Console,
    .level = spdlog::level::debug,
    .pattern = "[%H:%M:%S] [%n] %v",
    .enable_colors = true
});
```

### Switching Modes

```cpp
// Switch to syslog (typically after daemonization)
LoggerFactory::switchToSyslog("doipd", LOG_DAEMON);

// Switch back to console (e.g., for debugging)
LoggerFactory::switchToConsole(true);  // true = enable colors
```

### Getting Loggers

```cpp
// Get main logger
auto logger = LoggerFactory::get();
logger->info("Hello");

// Get specialized loggers
auto udp_logger = LoggerFactory::getUdp();
auto tcp_logger = LoggerFactory::getTcp();

// Or use macros (recommended)
LOG_DOIP_INFO("Server started");
LOG_UDP_DEBUG("Received packet");
LOG_TCP_ERROR("Connection failed");
```

### Setting Log Level

```cpp
// Set level for all loggers
LoggerFactory::setLevel(spdlog::level::debug);

// Set pattern for all loggers
LoggerFactory::setPattern("[%H:%M:%S.%e] %v");

// Check current mode
if (LoggerFactory::getMode() == LoggerMode::Syslog) {
    // Running in syslog mode
}
```

## Benefits of This Approach

### ✅ Separation of Concerns
- Daemonization logic separated from server logic
- Logger configuration independent of server
- Clean, testable code

### ✅ Flexibility
- Easy to switch between console and syslog
- Can run in foreground for debugging
- Works with systemd or traditional init

### ✅ Maintainability
- Standard Unix daemon patterns
- No surprises in constructor
- Easy to understand control flow

### ✅ Production Ready
- Proper PID file management
- Signal handling
- Graceful shutdown
- Resource cleanup

### ✅ Backward Compatible
- Existing logging macros still work
- No changes needed in most code
- Logger alias maintains compatibility

## Migration Checklist

- [ ] Replace `Logger.h` with `LoggerFactory` version
- [ ] Add `daemon_utils.h` and `daemon_utils.cpp`
- [ ] Remove `daemonize` flag from `ServerConfig`
- [ ] Remove `daemonize()` from DoIPServer constructor
- [ ] Update `main.cpp` with new initialization sequence
- [ ] Test in foreground mode
- [ ] Test in daemon mode
- [ ] Test syslog output
- [ ] Update systemd service file (if applicable)
- [ ] Update documentation

## Common Pitfalls to Avoid

### ❌ Don't: Daemonize after creating sockets
```cpp
// WRONG
DoIPServer server(config);
server.setupTcpSocket();
daemon::daemonize();  // ❌ Too late!
```

### ✅ Do: Daemonize before creating anything
```cpp
// CORRECT
daemon::daemonize();
LoggerFactory::switchToSyslog("doipd");
DoIPServer server(config);
server.setupTcpSocket();
```

### ❌ Don't: Switch logger before daemonization
```cpp
// WRONG
LoggerFactory::switchToSyslog("doipd");  // ❌ stdout still open
daemon::daemonize();  // Closes stdout - logger broken
```

### ✅ Do: Switch logger after daemonization
```cpp
// CORRECT
daemon::daemonize();  // Close stdout
LoggerFactory::switchToSyslog("doipd");  // Now use syslog
```

### ❌ Don't: Use console logging in daemon mode
```cpp
// WRONG
if (daemon_mode) {
    daemon::daemonize();
    // Still using console logger - output lost!
}
```

### ✅ Do: Switch to syslog in daemon mode
```cpp
// CORRECT
if (daemon_mode) {
    daemon::daemonize();
    LoggerFactory::switchToSyslog("doipd");
}
```

## Conclusion

This implementation provides a professional, production-ready solution for:
1. Dynamic logger switching (console ↔ syslog)
2. Proper Unix daemon creation
3. Clean separation of concerns
4. Easy debugging and testing

The logger factory pattern ensures that your logging infrastructure adapts seamlessly to the execution context, while the daemon utilities follow Unix best practices for robust background service operation.

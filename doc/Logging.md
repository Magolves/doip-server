# Logging in libdoip

This library uses [spdlog](https://github.com/gabime/spdlog) for high-performance logging.

## Features

- **High Performance**: spdlog is one of the fastest C++ logging libraries
- **Thread Safe**: All logging operations are thread-safe
- **Multiple Log Levels**: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL
- **Formatted Output**: Support for fmt-style formatting
- **Configurable Patterns**: Customize log message format
- **Color Support**: Colored console output for better readability

## Usage

### Configuration

```cpp
#include "Logger.h"

// Set log level (only messages at this level or higher will be shown)
doip::Logger::setLevel(spdlog::level::debug);

// Set custom pattern
doip::Logger::setPattern("[%H:%M:%S] [%^%l%$] %v");

// Available levels: trace, debug, info, warn, err, critical, off
```

### Pattern Format

The default pattern is: `[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v`

Common pattern flags:
- `%Y` - Year (4 digits)
- `%m` - Month (01-12)
- `%d` - Day (01-31)
- `%H` - Hour (00-23)
- `%M` - Minute (00-59)
- `%S` - Second (00-59)
- `%e` - Milliseconds (000-999)
- `%n` - Logger name
- `%l` - Log level
- `%^` - Start color range
- `%$` - End color range
- `%v` - The actual message


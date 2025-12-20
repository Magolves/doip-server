/**
 * @file main.cpp
 * @brief Example DoIP server implementation with daemon mode and dynamic logging
 * 
 * This demonstrates the proper sequence:
 * 1. Parse arguments
 * 2. Initialize logger (console mode)
 * 3. Daemonize (if requested)
 * 4. Switch logger to syslog (if daemon mode)
 * 5. Construct server
 * 6. Run server
 * 
 * Compile:
 *   g++ -std=c++17 main.cpp DoIPServer.cpp -lspdlog -pthread -o doipd
 * 
 * Usage:
 *   ./doipd                    # Foreground mode with console logging
 *   ./doipd --daemon           # Daemon mode with syslog
 *   ./doipd -d --verbose       # Daemon mode with debug logging
 */

#include "DoIPServer.h"
#include "Logger.h"
#include "daemon_utils.h"

#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <syslog.h>

using namespace doip;

// ============================================================================
// Global server pointer for signal handler
// ============================================================================
static std::unique_ptr<DoIPServer> g_server = nullptr;
static volatile sig_atomic_t g_shutdown_requested = 0;

// ============================================================================
// Signal handler for graceful shutdown
// ============================================================================
void signal_handler(int sig) {
    g_shutdown_requested = 1;
    
    // Log the signal (works in both console and syslog mode)
    if (sig == SIGTERM) {
        LOG_DOIP_INFO("Received SIGTERM, initiating graceful shutdown");
    } else if (sig == SIGINT) {
        LOG_DOIP_INFO("Received SIGINT, initiating graceful shutdown");
    }
    
    // Note: We don't call server->stop() here because signal handlers
    // should do minimal work. We'll check g_shutdown_requested in main loop.
}

// ============================================================================
// Command line argument structure
// ============================================================================
struct CommandLineArgs {
    bool daemon_mode = false;
    bool verbose = false;
    std::string pidfile = "/var/run/doipd.pid";
    std::string vin = "WAUZZZ8V9KA123456";
    uint16_t logical_address = 0x0028;
    spdlog::level::level_enum log_level = spdlog::level::info;
};

// ============================================================================
// Parse command line arguments
// ============================================================================
CommandLineArgs parseArguments(int argc, char* argv[]) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--daemon" || arg == "-d") {
            args.daemon_mode = true;
        }
        else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
            args.log_level = spdlog::level::debug;
        }
        else if (arg == "--trace") {
            args.log_level = spdlog::level::trace;
        }
        else if (arg == "--pidfile" && i + 1 < argc) {
            args.pidfile = argv[++i];
        }
        else if (arg == "--vin" && i + 1 < argc) {
            args.vin = argv[++i];
        }
        else if (arg == "--address" && i + 1 < argc) {
            args.logical_address = static_cast<uint16_t>(std::stoi(argv[++i], nullptr, 0));
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "\n"
                      << "Options:\n"
                      << "  -d, --daemon              Run as daemon (logs to syslog)\n"
                      << "  -v, --verbose             Enable verbose logging (debug level)\n"
                      << "  --trace                   Enable trace logging (very verbose)\n"
                      << "  --pidfile <path>          PID file path (default: /var/run/doipd.pid)\n"
                      << "  --vin <VIN>               Vehicle VIN (17 chars)\n"
                      << "  --address <addr>          Logical address (hex, e.g., 0x0028)\n"
                      << "  -h, --help                Show this help\n"
                      << "\n"
                      << "Examples:\n"
                      << "  " << argv[0] << "                           # Foreground mode\n"
                      << "  " << argv[0] << " --daemon                  # Daemon with syslog\n"
                      << "  " << argv[0] << " -d --verbose              # Daemon with debug logs\n"
                      << "  " << argv[0] << " --vin WBADT43452G123456   # Custom VIN\n"
                      << std::endl;
            exit(0);
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n"
                      << "Use --help for usage information" << std::endl;
            exit(1);
        }
    }
    
    return args;
}

// ============================================================================
// Main function
// ============================================================================
int main(int argc, char* argv[]) {
    // ------------------------------------------------------------------------
    // STEP 1: Parse command line arguments
    // ------------------------------------------------------------------------
    CommandLineArgs args = parseArguments(argc, argv);
    
    // ------------------------------------------------------------------------
    // STEP 2: Initialize logger in CONSOLE mode (before daemonization)
    // ------------------------------------------------------------------------
    LoggerConfig logger_config;
    logger_config.mode = LoggerMode::Console;
    logger_config.level = args.log_level;
    logger_config.enable_colors = LoggerFactory::colorsSupported();
    
    LoggerFactory::initialize(logger_config);
    
    LOG_DOIP_INFO("DoIP Server starting...");
    LOG_DOIP_DEBUG("Log level: {}", spdlog::level::to_string_view(args.log_level));
    
    // ------------------------------------------------------------------------
    // STEP 3: Daemonize if requested (BEFORE creating server objects)
    // ------------------------------------------------------------------------
    if (args.daemon_mode) {
        LOG_DOIP_INFO("Daemonizing process...");
        
        if (!daemon::daemonize(args.pidfile.c_str())) {
            std::cerr << "Failed to daemonize" << std::endl;
            return EXIT_FAILURE;
        }
        
        // Parent process has exited at this point
        // We are now in the daemon child process
        
        // ------------------------------------------------------------------------
        // STEP 4: Switch logger to SYSLOG mode (after daemonization)
        // ------------------------------------------------------------------------
        LoggerFactory::switchToSyslog("doipd", LOG_DAEMON);
        LoggerFactory::setLevel(args.log_level);
        
        LOG_DOIP_INFO("DoIP daemon started successfully (PID: {})", getpid());
    } else {
        LOG_DOIP_INFO("Running in foreground mode (console logging)");
    }
    
    // ------------------------------------------------------------------------
    // STEP 5: Set up signal handlers
    // ------------------------------------------------------------------------
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);  // Ignore broken pipe
    
    // SIGHUP in daemon mode could reload configuration
    if (args.daemon_mode) {
        signal(SIGHUP, SIG_IGN);  // Or implement config reload
    }
    
    // ------------------------------------------------------------------------
    // STEP 6: Create server configuration
    // ------------------------------------------------------------------------
    ServerConfig server_config;
    server_config.loopback = false;
    server_config.announceCount = 3;
    server_config.announceInterval = 500;
    // Note: daemonize flag removed from ServerConfig
    
    LOG_DOIP_DEBUG("Server configuration:");
    LOG_DOIP_DEBUG("  Announce count: {}", server_config.announceCount);
    LOG_DOIP_DEBUG("  Announce interval: {} ms", server_config.announceInterval);
    LOG_DOIP_DEBUG("  Loopback mode: {}", server_config.loopback);
    
    // ------------------------------------------------------------------------
    // STEP 7: Construct the DoIP server
    // ------------------------------------------------------------------------
    try {
        g_server = std::make_unique<DoIPServer>(server_config);
        LOG_DOIP_INFO("DoIP server instance created");
    } catch (const std::exception& e) {
        LOG_DOIP_ERROR("Failed to create server: {}", e.what());
        
        if (args.daemon_mode) {
            unlink(args.pidfile.c_str());
        }
        return EXIT_FAILURE;
    }
    
    // ------------------------------------------------------------------------
    // STEP 8: Configure server identifiers
    // ------------------------------------------------------------------------
    
    // Set EID from MAC address
    if (!g_server->setDefaultEid()) {
        LOG_DOIP_WARN("Failed to set EID from MAC address, using default");
    } else {
        LOG_DOIP_DEBUG("EID set from MAC address");
    }
    
    // Set VIN
    g_server->setVin(args.vin);
    LOG_DOIP_INFO("VIN configured: {}", args.vin);
    
    // Set logical address
    g_server->setLogicalGatewayAddress(DoIPAddress(args.logical_address));
    LOG_DOIP_INFO("Logical address: 0x{:04X}", args.logical_address);
    
    // ------------------------------------------------------------------------
    // STEP 9: Initialize network sockets
    // ------------------------------------------------------------------------
    
    // Setup TCP socket
    if (!g_server->setupTcpSocket()) {
        LOG_DOIP_ERROR("Failed to setup TCP socket on port {}", DOIP_SERVER_TCP_PORT);
        
        if (args.daemon_mode) {
            unlink(args.pidfile.c_str());
        }
        return EXIT_FAILURE;
    }
    LOG_DOIP_SUCCESS("TCP socket listening on port {}", DOIP_SERVER_TCP_PORT);
    
    // Setup UDP socket
    if (!g_server->setupUdpSocket()) {
        LOG_DOIP_ERROR("Failed to setup UDP socket on port {}", DOIP_UDP_DISCOVERY_PORT);
        
        if (args.daemon_mode) {
            unlink(args.pidfile.c_str());
        }
        return EXIT_FAILURE;
    }
    LOG_DOIP_SUCCESS("UDP socket bound on port {}", DOIP_UDP_DISCOVERY_PORT);
    
    // ------------------------------------------------------------------------
    // STEP 10: Server is ready - announce to the world!
    // ------------------------------------------------------------------------
    LOG_DOIP_HIGHLIGHT("==================================================");
    LOG_DOIP_HIGHLIGHT("  DoIP Server Ready");
    LOG_DOIP_HIGHLIGHT("  VIN: {}", args.vin);
    LOG_DOIP_HIGHLIGHT("  Logical Address: 0x{:04X}", args.logical_address);
    LOG_DOIP_HIGHLIGHT("  TCP Port: {}", DOIP_SERVER_TCP_PORT);
    LOG_DOIP_HIGHLIGHT("  UDP Port: {}", DOIP_UDP_DISCOVERY_PORT);
    LOG_DOIP_HIGHLIGHT("  Mode: {}", args.daemon_mode ? "Daemon (syslog)" : "Foreground (console)");
    LOG_DOIP_HIGHLIGHT("==================================================");
    
    // ------------------------------------------------------------------------
    // STEP 11: Main event loop - wait for shutdown signal
    // ------------------------------------------------------------------------
    while (!g_shutdown_requested && g_server->isRunning()) {
        sleep(1);  // Simple polling - could use condition variable instead
    }
    
    // ------------------------------------------------------------------------
    // STEP 12: Graceful shutdown
    // ------------------------------------------------------------------------
    LOG_DOIP_INFO("Shutting down server...");
    
    // Server cleanup happens automatically via unique_ptr destructor
    g_server.reset();
    
    LOG_DOIP_INFO("Server shutdown complete");
    
    // ------------------------------------------------------------------------
    // STEP 13: Cleanup daemon resources
    // ------------------------------------------------------------------------
    if (args.daemon_mode) {
        // Remove PID file
        if (unlink(args.pidfile.c_str()) == 0) {
            LOG_DOIP_DEBUG("PID file removed: {}", args.pidfile);
        }
    }
    
    LOG_DOIP_INFO("DoIP server terminated cleanly");
    
    return EXIT_SUCCESS;
}

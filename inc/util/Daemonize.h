#ifndef DAEMONIZE_H
#define DAEMONIZE_H

namespace doip {
namespace daemon {

/**
 * @brief Daemonize the current process using the standard double-fork technique
 *
 * This function performs the following steps:
 * 1. First fork() - create child process and exit parent
 * 2. setsid() - become session leader, detach from controlling terminal
 * 3. Ignore SIGHUP
 * 4. Second fork() - ensure daemon can't reacquire controlling terminal
 * 5. Change working directory to /
 * 6. Set file creation mask (umask)
 * 7. Close all file descriptors
 * 8. Redirect stdin/stdout/stderr to /dev/null
 * 9. Write PID file (if specified)
 *
 * CRITICAL: This must be called BEFORE:
 * - Creating any server objects
 * - Opening sockets
 * - Spawning threads
 * - Initializing logging (switch to syslog AFTER daemonization)
 * - Allocating significant resources
 *
 * @param pidfile Optional path to PID file (e.g., "/var/run/doipd.pid")
 * @return true in child process on success, false on error
 *         (parent process exits successfully and never returns)
 *
 * @note After successful daemonization:
 *       - Parent process has exited
 *       - Child process has no controlling terminal
 *       - stdin/stdout/stderr point to /dev/null
 *       - Working directory is /
 *       - Process is session leader
 *
 * Example usage:
 * @code
 *   if (daemon_mode) {
 *       if (!daemon::daemonize("/var/run/myapp.pid")) {
 *           std::cerr << "Daemonization failed" << std::endl;
 *           return 1;
 *       }
 *       // Now in daemon process - switch logger to syslog
 *       LoggerFactory::switchToSyslog("myapp");
 *   }
 * @endcode
 */
bool daemonize(const char *pidfile = nullptr);

/**
 * @brief Check if a daemon with the given PID file is already running
 *
 * @param pidfile Path to PID file
 * @return true if daemon is running, false otherwise
 */
bool isRunning(const char *pidfile);

/**
 * @brief Remove PID file (call during shutdown)
 *
 * @param pidfile Path to PID file
 * @return true on success, false on error
 */
bool removePidFile(const char *pidfile);

} // namespace daemon
} // namespace doip

#endif /* DAEMON_H */

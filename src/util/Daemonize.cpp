/**
 * @file daemon_utils.cpp
 * @brief Implementation of Unix daemon utilities
 */

#include "util/Daemonize.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace doip {
namespace daemon {

bool daemonize(const char *pidfile) {
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "First fork() failed: " << strerror(errno) << std::endl;
        return false;
    }

    if (pid > 0) {
        // Parent process - exit successfully
        // Child continues in background
        _exit(EXIT_SUCCESS);
    }

    // STEP 2: Become session leader and detach from controlling terminal
    if (setsid() < 0) {
        std::cerr << "setsid() failed: " << strerror(errno) << std::endl;
        return false;
    }

    // Ignore SIGHUP (sent when session leader exits)
    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if (pid < 0) {
        std::cerr << "Second fork() failed: " << strerror(errno) << std::endl;
        return false;
    }

    if (pid > 0) {
        // First child exits - second child continues as daemon
        _exit(EXIT_SUCCESS);
    }

    // ========================================================================
    umask(0);

    if (chdir("/") < 0) {
        std::cerr << "chdir(\"/\") failed: " << strerror(errno) << std::endl;
        return false;
    }

    long max_fd = sysconf(_SC_OPEN_MAX);
    if (max_fd < 0) {
        max_fd = 1024; // Fallback if sysconf fails
    }

    // Close all file descriptors
    for (long fd = 0; fd < max_fd; fd++) {
        close(fd);
    }

    int null_fd = open("/dev/null", O_RDWR);

    if (null_fd < 0) {
        // Can't write to stderr anymore (it's closed), so just return
        return false;
    }

    if (dup2(null_fd, STDIN_FILENO) < 0) {
        return false;
    }

    if (dup2(null_fd, STDOUT_FILENO) < 0) {
        return false;
    }

    if (dup2(null_fd, STDERR_FILENO) < 0) {
        return false;
    }
    if (dup2(null_fd, STDERR_FILENO) < 0) {
        return false;
    }

    if (null_fd > STDERR_FILENO) {
        close(null_fd);
    }

    // Write PID file (if requested)
    if (pidfile != nullptr) {
        // Check if daemon is already running
        if (isRunning(pidfile)) {
            // Can't use stderr anymore, but we return false
            return false;
        }

        FILE *pf = fopen(pidfile, "w");
        if (pf == nullptr) {
            return false;
        }

        fprintf(pf, "%d\n", getpid());
        fclose(pf);

        // Set appropriate permissions (readable by all, writable by owner)
        chmod(pidfile, 0644);
    }

    return true;
}

bool isRunning(const char *pidfile) {
    if (pidfile == nullptr) {
        return false;
    }

    FILE *pf = fopen(pidfile, "r");
    if (pf == nullptr) {
        // PID file doesn't exist - daemon not running
        return false;
    }

    pid_t pid;
    if (fscanf(pf, "%d", &pid) != 1) {
        fclose(pf);
        return false;
    }
    fclose(pf);

    if (kill(pid, 0) == 0) {
        // Process exists
        return true;
    }

    // Process doesn't exist
    if (errno == ESRCH) {
        // Stale PID file - remove it
        unlink(pidfile);
        return false;
    }

    // Some other error - assume running
    return true;
}

bool removePidFile(const char *pidfile) {
    if (pidfile == nullptr) {
        return true;
    }

    if (unlink(pidfile) == 0) {
        return true;
    }

    // File might not exist - that's OK
    if (errno == ENOENT) {
        return true;
    }

    return false;
}

} // namespace daemon
} // namespace doip

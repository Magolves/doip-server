#ifndef SOCKET_H
#define SOCKET_H

#include <unistd.h>
#include <utility>

namespace doip {

/**
 * @brief RAII wrapper for POSIX file descriptors (sockets)
 *
 * Ensures sockets are properly closed when going out of scope,
 * preventing resource leaks and improving exception safety.
 */
class Socket {
  public:
    /**
     * @brief Default constructor - creates invalid socket
     */
    Socket() = default;

    /**
     * @brief Construct from existing file descriptor
     * @param fd File descriptor to wrap (takes ownership)
     */
    explicit Socket(int fd) : m_fd(fd) {}

    /**
     * @brief Destructor - automatically closes socket
     */
    ~Socket() noexcept {
        close();
    }

    // Disable copy (sockets can't be copied)
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    /**
     * @brief Move constructor
     */
    Socket(Socket &&other) noexcept : m_fd(other.m_fd) {
        other.m_fd = -1;
    }

    /**
     * @brief Move assignment
     */
    Socket &operator=(Socket &&other) noexcept {
        if (this != &other) {
            close(); // Close current socket first
            m_fd = other.m_fd;
            other.m_fd = -1;
        }
        return *this;
    }

    /**
     * @brief Get the underlying file descriptor
     * @return File descriptor, or -1 if invalid
     */
    [[nodiscard]]
    int get() const noexcept {
        return m_fd;
    }

    /**
     * @brief Check if socket is valid
     * @return true if socket is valid (fd >= 0)
     */
    [[nodiscard]]
    bool valid() const noexcept {
        return m_fd >= 0;
    }

    /**
     * @brief Release ownership of the file descriptor
     * @return The file descriptor (caller must close it)
     */
    [[nodiscard]]
    int release() noexcept {
        int fd = m_fd;
        m_fd = -1;
        return fd;
    }

    /**
     * @brief Explicitly close the socket
     *
     * Safe to call multiple times (idempotent).
     * Automatically called by destructor.
     */
    void close() noexcept {
        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }
    }

    /**
     * @brief Reset to a new file descriptor
     * @param fd New file descriptor (takes ownership)
     */
    void reset(int fd = -1) noexcept {
        close();
        m_fd = fd;
    }

    /**
     * @brief Implicit conversion to bool for validity checks
     */
    explicit operator bool() const noexcept {
        return valid();
    }

  private:
    int m_fd{-1};
};

} // namespace doip

#endif /* SOCKET_H */

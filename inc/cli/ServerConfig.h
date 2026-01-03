#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include "Vin.h"
#include "DoIPIdentifiers.h"
#include "DoIPAddress.h"

namespace doip {
/**
 * @brief Server configuration structure used to initialize a DoIP server.
 */
struct ServerConfig {
    // EID and GID as fixed identifiers (6 bytes). Default: zeros.
    EntityId eid = EntityId::Zero;
    GroupId gid = GroupId::Zero;

    // VIN as fixed identifier (17 bytes). Default: zeros.
    Vin vin = Vin::Zero;

    // Logical/server address (default 0x0028)
    DoIPAddress logicalAddress = DoIPAddress(0x0028);

    // Use loopback announcements instead of broadcast
    bool loopback = false;

    // Run the server as a daemon by default
    bool daemonize = false;

    int announceCount = 3;               // Default Value = 3
    unsigned int announceInterval = 500; // Default Value = 500ms
};

} // namespace doip
#endif /* SERVERCONFIG_H */

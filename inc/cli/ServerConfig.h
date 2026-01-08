#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include "DoIPAddress.h"
#include "DoIPIdentifiers.h"
#include "DoIPTimes.h"
#include "Vin.h"

namespace doip {

struct ServerProperties {
    /**
     * @brief Vehicle Identification Number (VIN) consisting of 17 characters.
     * See ISO 3779 for details.
     */
    Vin vin = Vin::Zero;
    /**
     * @brief Entity Identifier (EID) - 6-byte unique identifier for the vehicle.
     */
    EntityId eid = EntityId::Zero;
    /**
     * @brief Group Identifier (GID) - 6-byte identifier for vehicle group.
     */
    GroupId gid = GroupId::Zero;
    /**
     * @brief DoIP logical address of the server (gateway).
     */
    DoIPAddress logicalAddress = ZERO_ADDRESS;
};

/**
 * @brief Server configuration structure used to initialize a DoIP server.
 */
struct ServerConfig {
    /**
     * @brief Server properties (EID, GID, VIN, logical address).
     */
    ServerProperties properties;

    /**
     * @brief If true, use loopback interface for vehicle announcements.
     */
    bool loopback = false;

    /**
     * @brief If true, run the server as a daemon.
     */
    bool daemonize = false;

    /**
     * @brief Number of vehicle announcements to send upon startup.
     */
    int announceCount = 3;               // Default Value = 3

    /**
     * @brief Interval between vehicle announcements in milliseconds.
     */
    unsigned int announceInterval = times::server::VehicleAnnouncementInterval.count();
};

} // namespace doip
#endif /* SERVERCONFIG_H */

#ifndef EID_H
#define EID_H

#include "GenericFixedId.h"


namespace doip {

/**
 * @brief Entity Identifier (EID) - 6 bytes for unique entity identification
 */
using EntityId = GenericFixedId<6, false>;

/**
 * @brief Stream output operator for EntityId/GroupId
 *
 * @param os the operation stream
 * @param eid the EntityId/GroupId to output
 * @return std::ostream&  @ref {type}  ["{type}"]   //  @return Returns @c true in the case of success, @c false otherwise.
 */
inline std::ostream &operator<<(std::ostream &os, const EntityId &eid) {
    os << eid.toHexString();
    return os;
}

/**
 * @brief Group Identifier (GID) - 6 bytes for unique group identification
 */
using GroupId = GenericFixedId<6, false>;


} // namespace doip

#endif /* EID_H */

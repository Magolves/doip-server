#ifndef IUDSMODEL_H
#define IUDSMODEL_H

#include "util/ByteArray.h"

#include <string_view>

namespace doip::uds {

class IUdsModel {
public:
    virtual ~IUdsModel() = default;

    /**
     * @brief Get the model name.
     * @return model name string.
     */
    virtual std::string_view getModelName() const = 0;


};

} // namespace doip::uds

#endif /* IUDSMODEL_H */

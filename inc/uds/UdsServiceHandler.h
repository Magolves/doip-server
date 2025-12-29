#ifndef IUDSSERVICEHANDLER_H
#define IUDSSERVICEHANDLER_H

#include "IUdsModel.h"
#include "UdsResponseCode.h"
#include "UdsServices.h"
#include "UdsTypes.h"
#include "util/ByteArray.h"
#include <memory>

namespace doip::uds {

using UdsResponse = std::pair<UdsResponseCode, ByteArray>;

inline std::ostream &operator<<(std::ostream &os, const UdsResponse &response) {
    std::ios_base::fmtflags flags(os.flags());

    os << response.first << " [";
    os << std::hex << std::uppercase << std::setw(2) << std::setfill('0');

    for (size_t i = 0; i < response.second.size(); ++i) {
        if (i > 0) {
            os << '.';
        }
        os << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
           << static_cast<unsigned int>(response.second[i]);
    }

    os.flags(flags);
    return os;
}

class UdsServiceHandler {
  public:
    virtual ~UdsServiceHandler() = default;
    virtual UdsResponse handle(const ByteArray &request, const UniqueUdsModelPtr &model) = 0;

  protected:
    virtual UdsResponse makeResponse(const ByteArray &request, const ByteArray &data = {});
    virtual UdsResponse makeNegativeResponse(UdsResponseCode code, const ByteArray &request) const;
};

using UniqueUdsServiceHandlerPtr = std::unique_ptr<UdsServiceHandler>;
using IUdsServiceHandlerPtr = std::unique_ptr<UdsServiceHandler>;

} // namespace doip::uds

#endif // IUDSSERVICEHANDLER_H

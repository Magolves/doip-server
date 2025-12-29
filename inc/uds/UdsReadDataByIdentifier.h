#pragma once

#include <unordered_map>
#include <cstdint>

#include "IUdsServiceHandler.h"
#include "UdsServices.h"

namespace doip::uds {

class ReadDataByIdentifierHandler : public IUdsServiceHandler {
public:
    ~ReadDataByIdentifierHandler() override = default;
    UdsResponse handle(const ByteArray& request) override;
protected:
    using IUdsServiceHandler::makeResponse;
    using IUdsServiceHandler::makeNegativeResponse;
    virtual UdsResponse makeResponse(const ByteArray& request, const ByteArray& data) override;
private:
    std::unordered_map<uint16_t, ByteArray> m_didValues;
};

} // namespace doip::uds

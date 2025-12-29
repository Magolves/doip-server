#ifndef UDSDEFAULTMODEL_H
#define UDSDEFAULTMODEL_H

#include "IUdsModel.h"

namespace doip::uds {

class UdsDefaultModel : public IUdsModel {
  public:
    UdsDefaultModel() = default;
    ~UdsDefaultModel() override = default;

    std::string_view getModelName() const override { return "UDS Default Model"; }



    bool supportsDataByIdentifier(uds_did did) const override {
        (void)did;
        return true;
    }

    UdsResponseCode getDataByIdentfier(uds_did did, ByteArray &data, size_t offset = 0) const override {
        (void)offset;
        data.clear();
        data.writeU16At(0, did);
        return UdsResponseCode::PositiveResponse;
    }

    UdsResponseCode setDataByIdentfier(uds_did did, const ByteArray &data, size_t offset = 0) const override {
        (void)did;
        (void)data;
        (void)offset;
        return UdsResponseCode::PositiveResponse;
    }


};

} // namespace doip::uds

#endif /* UDSDEFAULTMODEL_H */

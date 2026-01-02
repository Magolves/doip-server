#ifndef UDSDEFAULTMODEL_H
#define UDSDEFAULTMODEL_H

#include "IUdsModel.h"
#include "DoIPIdentifiers.h"
#include "UdsTransferInfo.h"
#include "UdsDataIdentifiers.h"

namespace doip::uds {

class UdsDefaultModel : public IUdsModel {
  public:
    UdsDefaultModel() = default;
    ~UdsDefaultModel() override = default;

    std::string_view getModelName() const override { return "UDS Default Model"; }

    const UdsTransferInfo& getTransferInfo() const {
        return m_transferInfo;
    }

    virtual UdsResponseCode requestDownload(uint32_t memoryAddress, uint32_t memorySize, const ByteArray &transferParameters) override {
        return m_transferInfo.startTransfer(TransferMode::Download, memoryAddress, memorySize, MAX_UDS_MESSAGE_LENGTH, transferParameters);
    }

    virtual UdsResponseCode transferData(uint8_t blockSequenceCounter, const ByteArray &data) override {
        (void)blockSequenceCounter;

        return m_transferInfo.recordBlockTransfer(data);
    }

    virtual UdsResponseCode requestTransferExit() override {
        return m_transferInfo.endTransfer();
    }


    bool supportsDataByIdentifier(uds_did did) const override {
        (void)did;
        return true;
    }

    UdsResponseCode getDataByIdentfier(uds_did did, ByteArray &data, size_t offset = 0) const override {
        (void)offset;
        data.writeU16(did);

        if (populateDidData(did, data)) {
            return UdsResponseCode::PositiveResponse;
        }

        return UdsResponseCode::RequestOutOfRange;
    }

    UdsResponseCode setDataByIdentfier(uds_did did, const ByteArray &data, size_t offset = 0) override {
        (void)did;
        (void)data;
        (void)offset;

        if (updateDidData(did, data, offset)) {
            return UdsResponseCode::PositiveResponse;
        }
        return UdsResponseCode::RequestOutOfRange;
    }

    private:
        UdsTransferInfo m_transferInfo{};

        DoIpVin m_vin{"WVWZZZ1JZ4W012345"}; // Volkswagen (fictional model)

        bool populateDidData(uds_did did, ByteArray &data) const {
            switch (static_cast<UdsDataIdentifier>(did)) {
                case UdsDataIdentifier::VIN: {
                    // VIN is 17 bytes
                    data.writeString(m_vin.toString());
                    return true;
                }
                default:
                    break;
            }
            return false;
        }

        bool updateDidData(uds_did did, const ByteArray &data, size_t offset) {
            switch (static_cast<UdsDataIdentifier>(did)) {
                case UdsDataIdentifier::VIN: {
                    if (data.size() - offset < DoIpVin::VIN_LENGTH) {
                        return false;
                    }
                    m_vin = DoIpVin(std::string(reinterpret_cast<const char *>(&data[offset]), DoIpVin::VIN_LENGTH));
                    return true;
                }
                default:
                    break;
            }
            return false;
        }
};

} // namespace doip::uds

#endif /* UDSDEFAULTMODEL_H */

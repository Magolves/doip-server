#ifndef UDSTRANSFERINFO_H
#define UDSTRANSFERINFO_H

#include "util/ByteArray.h"
#include "uds/UdsTypes.h"
#include "uds/UdsResponseCode.h"

namespace doip::uds {

struct UdsTransferInfo {
    UdsTransferInfo() = default;

    TransferMode getTransferMode() const {
        return m_transferMode;
    }

    [[nodiscard]]
    UdsResponseCode startTransfer(TransferMode mode, uint32_t address, uint32_t size, uint32_t blockSize, const ByteArray &parameters) {
        if (m_transferMode != TransferMode::None) {
            return UdsResponseCode::ConditionsNotCorrect; //  Transfer already in progress
        }

        if (blockSize == 0) {
            return UdsResponseCode::RequestOutOfRange; // Invalid block size
        }

        if (size == 0) {
            return UdsResponseCode::RequestOutOfRange; // Invalid size
        }

        resetTransfer();

        m_transferMode = mode;
        m_memoryAddress = address;
        m_memorySize = size;
        m_transferParameters = parameters;
        m_blockSize = blockSize;
        m_expectedBlocks = (size + blockSize - 1) / blockSize; // ceiling division

        return UdsResponseCode::PositiveResponse;
    }

    [[nodiscard]]
    UdsResponseCode recordBlockTransfer(uint32_t bytesTransferred) {
        if (m_transferMode == TransferMode::None) {
            resetTransfer();
            return UdsResponseCode::RequestSequenceError;
        }

        ++m_transferredBlocks;
        m_transferredBytes += bytesTransferred;

        return UdsResponseCode::PositiveResponse;
    }

    [[nodiscard]]
    UdsResponseCode endTransfer() {
        if (m_transferMode == TransferMode::None) {
            return UdsResponseCode::RequestSequenceError; // No active transfer
        }

        resetTransfer();
        return m_expectedBlocks == m_transferredBlocks ? UdsResponseCode::PositiveResponse : UdsResponseCode::ConditionsNotCorrect;
    }

    /**
     * @brief Abort the current transfer
     */
    void abortTransfer() {
        resetTransfer();
    }

    [[nodiscard]]
    uint32_t getTransferredBytes() const {
        return m_transferredBytes;
    }

    [[nodiscard]]
    uint32_t getTransferredBlocks() const {
        return m_transferredBlocks;
    }

    [[nodiscard]]
    bool isTransferInProgress() const {
        return m_transferMode != TransferMode::None;
    }

    [[nodiscard]]
    uint32_t getMemoryAddress() const {
        return m_memoryAddress;
    }

    [[nodiscard]]
    uint32_t getMemorySize() const {
        return m_memorySize;
    }

    [[nodiscard]]
    const ByteArray& getTransferParameters() const {
        return m_transferParameters;
    }

        [[nodiscard]]
    uint32_t getExpectedBlocks() const {
        return m_expectedBlocks;
    }

    [[nodiscard]]
    uint32_t getBlockSize() const {
        return m_blockSize;
    }

    private:
    TransferMode m_transferMode = TransferMode::None;
    uint32_t m_memoryAddress = 0;
    uint32_t m_memorySize = 0;
    ByteArray m_transferParameters{};
    uint32_t m_blockSize = 0;
    uint32_t m_expectedBlocks = 0;
    uint32_t m_transferredBlocks = 0;
    uint32_t m_transferredBytes = 0;

    void resetTransfer() {
        m_transferMode = TransferMode::None;
        m_memoryAddress = 0;
        m_memorySize = 0;
        m_transferParameters.clear();
        m_blockSize = 0;
        m_expectedBlocks = 0;
        m_transferredBlocks = 0;
        m_transferredBytes = 0;
    }
};

}

#endif /* UDSTRANSFERINFO_H */

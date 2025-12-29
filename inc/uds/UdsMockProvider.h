#pragma once
#include "ByteArray.h"
#include "IDownstreamProvider.h"
#include "ThreadSafeQueue.h"
#include "uds/UdsMock.h"
#include <atomic>
#include <thread>

namespace doip::uds {

using namespace std::chrono_literals;
using namespace doip;

class UdsMockProvider : public IDownstreamProvider {
  public:
    UdsMockProvider() {
        m_uds.registerDefaultServices();
    }

    ~UdsMockProvider() override = default;

    // void start() override {
    // }

    // void stop() override {
    // }

    void sendRequest(const ByteArray request, DownstreamCallback cb) override {
        if (!cb)
            return;

        auto start_ts = std::chrono::steady_clock::now();

        // Synchronous UDS processing
        ByteArray rsp = m_uds.handleDiagnosticRequest(request);

        auto end_ts = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - start_ts);

        DownstreamResponse dr;
        dr.payload = rsp;
        dr.latency = latency;
        dr.status = DownstreamStatus::Handled;
        cb(dr); // Invoke the callback immediately
    }

    virtual std::string_view getProviderName() const override {
        return "UdsMockProvider";
    }

  private:
    uds::UdsMock m_uds;
    uint16_t m_p2_ms = 2000;
    uint16_t m_p2star_10ms = 400;
};

} // namespace doip::uds

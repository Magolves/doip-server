#ifndef UDSSERVERMODEL_H
#define UDSSERVERMODEL_H

#include "DoIPDownstreamServerModel.h"
#include "DoIPServerModel.h"
#include "uds/UdsMockProvider.h"
#include "uds/UdsResponseCode.h"
#include "util/ThreadSafeQueue.h"

namespace doip::uds {

/**
 * @brief UDS DoIP Server Model with UdsMockProvider
 *
 */
class UdsServerModel : public DoIPDownstreamServerModel {
  public:
    UdsServerModel() : DoIPDownstreamServerModel("uds", m_uds) {
        // Customize callbacks if needed
    }

    virtual std::string_view getModelName() const override { return "UdsServerModel"; }

  private:
    uds::UdsMockProvider m_uds;
};
} // namespace doip::uds

#endif /* UDSSERVERMODEL_H */

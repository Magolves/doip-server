
/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */

#ifndef SOCKETCANSERVERMODEL_H
#define SOCKETCANSERVERMODEL_H


#include "DoIPDownstreamServerModel.h"
#include "can/CanIsoTpProvider.h"
#include "ThreadSafeQueue.h"

using namespace doip;
using namespace doip::can;

// Helper base class to ensure provider is initialized before DoIPDownstreamServerModel
struct CanProviderHolder {
    can::CanIsoTpProvider provider;

    CanProviderHolder(const std::string& interfaceName, uint32_t tx_address, uint32_t rx_address)
        : provider(interfaceName, tx_address, rx_address) {}
};

class CanIsoTpServerModel : private CanProviderHolder, public DoIPDownstreamServerModel {
  public:
    CanIsoTpServerModel(const std::string& interfaceName, uint32_t tx_address, uint32_t rx_address)
        : CanProviderHolder(interfaceName, tx_address, rx_address),
          DoIPDownstreamServerModel("isotp", provider) {
        // Customize callbacks if needed
    }
};


#endif /* SOCKETCANSERVERMODEL_H */

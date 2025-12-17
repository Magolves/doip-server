/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */

#ifndef DoIPServerMODEL_H
#define DoIPServerMODEL_H

#include "DoIPServerModel.h"
#include "DoIPDownstreamServerModel.h"
#include "ThreadSafeQueue.h"
#include "uds/UdsMockProvider.h"
#include "uds/UdsResponseCode.h"

using namespace doip;
using namespace doip::uds;

class DoIPServerModel : public DoIPDownstreamServerModel {
  public:
    DoIPServerModel() : DoIPDownstreamServerModel("exmod", m_uds) {
        // Customize callbacks if needed

    }

  private:
    uds::UdsMockProvider m_uds;
};

#endif /* DoIPServerMODEL_H */

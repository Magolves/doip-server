/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */

#ifndef EXAMPLEDOIPSERVERMODEL_H
#define EXAMPLEDOIPSERVERMODEL_H

#ifndef DoIPServerMODEL_H
#define DoIPServerMODEL_H

#include "DoIPServerModel.h"
#include "DoIPDownstreamServerModel.h"
#include "util/ThreadSafeQueue.h"
#include "uds/UdsMockProvider.h"
#include "uds/UdsResponseCode.h"

using namespace doip;
using namespace doip::uds;

class ExampleDoIPServerModel : public DoIPDownstreamServerModel {
  public:
    ExampleDoIPServerModel() : DoIPDownstreamServerModel("uds-session", m_uds) {
        // Customize callbacks if needed

    }

    virtual std::string_view getModelName() const override { return "ExampleDoIPServerModel"; }

  private:
    uds::UdsMockProvider m_uds;
};

#endif /* DoIPServerMODEL_H */


#endif /* EXAMPLEDOIPSERVERMODEL_H */

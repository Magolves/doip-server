#ifndef LAMBDAUDSHANDLER_H
#define LAMBDAUDSHANDLER_H

#include "UdsServiceHandler.h"
#include <functional>

namespace doip::uds {

class LambdaUdsHandler : public UdsServiceHandler {
public:
    using Fn = std::function<UdsResponse(const ByteArray &, const UniqueUdsModelPtr&)>;
    explicit LambdaUdsHandler(Fn fn) : m_fn(std::move(fn)) {}

    UdsResponse handle(const ByteArray &request, const UniqueUdsModelPtr& model) override { return m_fn(request, model); }

private:
    Fn m_fn;
};

} // namespace doip::uds

#endif // LAMBDAUDSHANDLER_H

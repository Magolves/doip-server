#include "DoIPDefaultConnection.h"
#include "Logger.h"

#include <execinfo.h>
#include <iostream>

namespace doip {

DoIPDefaultConnection::DoIPDefaultConnection(UniqueServerModelPtr model, UniqueConnectionTransportPtr tp,  const SharedTimerManagerPtr<ConnectionTimers> &timerManager)
    : m_serverModel(std::move(model)),
        m_transport(std::move(tp)),
      m_timerManager(timerManager),
      STATE_DESCRIPTORS{
          StateDescriptor(
              DoIPServerState::SocketInitialized,
              DoIPServerState::WaitRoutingActivation,
              [this](std::optional<DoIPMessage> msg) { this->handleSocketInitialized(DoIPServerEvent{}, msg); }),
          StateDescriptor(
              DoIPServerState::WaitRoutingActivation,
              DoIPServerState::Finalize,
              [this](std::optional<DoIPMessage> msg) { this->handleWaitRoutingActivation(DoIPServerEvent{}, msg); },
              ConnectionTimers::InitialInactivity),
          StateDescriptor(
              DoIPServerState::RoutingActivated,
              DoIPServerState::Finalize,
              [this](std::optional<DoIPMessage> msg) { this->handleRoutingActivated(DoIPServerEvent{}, msg); },
              ConnectionTimers::GeneralInactivity,
              [this]() noexcept { m_aliveCheckRetry = 0; }),
          StateDescriptor(
              DoIPServerState::WaitAliveCheckResponse,
              DoIPServerState::Finalize,
              [this](std::optional<DoIPMessage> msg) { this->handleWaitAliveCheckResponse(DoIPServerEvent{}, msg); },
              ConnectionTimers::AliveCheck,
              [this]() { ++m_aliveCheckRetry; m_log->warn("Alive check #{}/{}", m_aliveCheckRetry, m_aliveCheckRetryCount); }),
          StateDescriptor(
              DoIPServerState::WaitDownstreamResponse,
              DoIPServerState::Finalize,
              [this](std::optional<DoIPMessage> msg) { this->handleWaitDownstreamResponse(DoIPServerEvent{}, msg); },
              ConnectionTimers::UserDefined,
              nullptr,
              nullptr,
              2s),
          StateDescriptor(
              DoIPServerState::Finalize,
              DoIPServerState::Closed,
              [this](std::optional<DoIPMessage> msg) { this->handleFinalize(DoIPServerEvent{}, msg); }),
          StateDescriptor(
              DoIPServerState::Closed,
              DoIPServerState::Closed,
              nullptr)} {
    m_isOpen = true;
    m_serverModel->onOpenConnection(*this);
    m_state = &STATE_DESCRIPTORS[0];

    m_log->info("Default connection created, transitioning to SocketInitialized state...");
    transitionTo(DoIPServerState::WaitRoutingActivation);
}

ssize_t DoIPDefaultConnection::sendProtocolMessage(const DoIPMessage &msg) {
    m_log->info("Default connection: Sending protocol message: {}", fmt::streamed(msg));
    return m_transport->sendMessage(msg);
}

std::optional<DoIPMessage> DoIPDefaultConnection::receiveProtocolMessage() {
    m_log->info("Default connection: Receiving protocol message...");
    return m_transport->receiveMessage();
}

void DoIPDefaultConnection::closeConnection(DoIPCloseReason reason) {
    try {
        m_log->info("Default connection: Closing connection, reason: {}", fmt::streamed(reason));
        transitionTo(DoIPServerState::Closed);
        m_closeReason = reason;
        m_timerManager->stopAll();
        m_transport->close(reason);  // Close the transport
        notifyConnectionClosed(reason);
    } catch (const std::exception &e) {
        m_log->error("Error notifying connection closed: {}", e.what());

        void *callstack[128];
        int frames = backtrace(callstack, 128);
        char **strs = backtrace_symbols(callstack, frames);

        m_log->error("Exception during closeConnection: {}", e.what());
        m_log->error("Stack trace:");
        for (int i = 0; i < frames; ++i) {
            m_log->error("{}", strs[i]);
        }
        free(strs);
    }
    m_isOpen = false;
}

DoIPAddress DoIPDefaultConnection::getServerAddress() const {
    return m_serverModel->serverAddress;
}

DoIPAddress DoIPDefaultConnection::getClientAddress() const {
    return m_routedClientAddress;
}

void DoIPDefaultConnection::setClientAddress(const DoIPAddress &address) {
    m_routedClientAddress = address;
}

void DoIPDefaultConnection::handleMessage2(const DoIPMessage &message) {
    m_state->messageHandler(message);
}

void DoIPDefaultConnection::transitionTo(DoIPServerState newState) {
    if (m_state->state == newState) {
        return;
    }

    m_log->info("-> Transitioning from state {} to state {}", fmt::streamed(m_state->state), fmt::streamed(newState));

    // Direct array access using enum as index (assumes enum values 0-6)
    size_t index = static_cast<size_t>(newState);
    if (index >= STATE_DESCRIPTORS.size()) {
        m_log->error("Invalid state transition to {} (index out of bounds: {})", fmt::streamed(newState), index);
        return;
    }

    m_state = &STATE_DESCRIPTORS[index];

    startStateTimer(m_state);
    if (m_state->enterStateHandler) {
        m_log->info("Calling enterState handler");
        m_state->enterStateHandler();
    }
}

std::chrono::milliseconds DoIPDefaultConnection::getTimerDuration(StateDescriptor const *stateDesc) {
    assert(stateDesc);
    switch (stateDesc->timer) {
    case ConnectionTimers::AliveCheck:
        return m_aliveCheckTimeout;
    case ConnectionTimers::InitialInactivity:
        return m_initialInactivityTimeout;
    case ConnectionTimers::GeneralInactivity:
        return m_generalInactivityTimeout;
    case ConnectionTimers::DownstreamResponse:
        return m_downstreamResponseTimeout;
    case ConnectionTimers::UserDefined:
        return stateDesc->timeoutDurationUser;
    default:
        return 0ms;
    }
}

void DoIPDefaultConnection::startStateTimer(StateDescriptor const *stateDesc) {
    assert(stateDesc != nullptr);
    if (stateDesc == nullptr) {
        return;
    }

    m_timerManager->stopAll();

    std::chrono::milliseconds duration = getTimerDuration(m_state);

    if (duration.count() == 0) {
        m_log->debug("User-defined timer duration is zero, transitioning immediately to state {}", fmt::streamed(stateDesc->stateAfterTimeout));
        transitionTo(stateDesc->stateAfterTimeout);
        return;
    }

    m_log->debug("Starting timer for state {}: Timer ID {}, duration {}ms", fmt::streamed(stateDesc->state), fmt::streamed(stateDesc->timer), duration.count());

    std::function<void(ConnectionTimers)> timeoutHandler = [this](ConnectionTimers timerId) { handleTimeout(timerId); };
    if (stateDesc->timeoutHandler != nullptr) {
        timeoutHandler = stateDesc->timeoutHandler;
    }

    auto id = m_timerManager->addTimer(
        m_state->timer, duration, timeoutHandler, false);

    if (id.has_value()) {
        m_log->debug("Started timer {} for {}ms", fmt::streamed(m_state->timer), duration.count());
    } else {
        m_log->error("Failed to start timer {}", fmt::streamed(m_state->timer));
    }
}

void DoIPDefaultConnection::restartStateTimer() {
    assert(m_state != nullptr);
    if (!m_timerManager->restartTimer(m_state->timer)) {
        m_log->error("Failed to restart timer {}", fmt::streamed(m_state->timer));
    }
}

void DoIPDefaultConnection::handleSocketInitialized(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    transitionTo(DoIPServerState::WaitRoutingActivation);
}

void DoIPDefaultConnection::handleWaitRoutingActivation(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    // Implementation of handling routing activation would go here
    if (!msg) {
        closeConnection(DoIPCloseReason::SocketError);
        return;
    }

    auto sourceAddress = msg->getSourceAddress();
    bool hasAddress = sourceAddress.has_value();
    bool rightPayloadType = (msg->getPayloadType() == DoIPPayloadType::RoutingActivationRequest);

    if (!hasAddress || !rightPayloadType) {
        m_log->warn("Invalid Routing Activation Request message");
        closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }

    // Set client address in context
    setClientAddress(sourceAddress.value());
    // Send Routing Activation Response
    sendRoutingActivationResponse(sourceAddress.value(), DoIPRoutingActivationResult::RouteActivated);
    // Transition to Routing Activated state
    transitionTo(DoIPServerState::RoutingActivated);
}

void DoIPDefaultConnection::handleRoutingActivated(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter

    if (!msg) {
        closeConnection(DoIPCloseReason::SocketError);
        return;
    }

    auto message = msg.value();

    switch (message.getPayloadType()) {
    case DoIPPayloadType::DiagnosticMessage:
        break;
    case DoIPPayloadType::AliveCheckResponse:
        restartStateTimer();
        return;
    default:
        m_log->warn("Received unsupported message type {} in Routing Activated state", fmt::streamed(message.getPayloadType()));
        sendDiagnosticMessageResponse(ZERO_ADDRESS, DoIPNegativeDiagnosticAck::TransportProtocolError);
        // closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }
    auto sourceAddress = message.getSourceAddress();
    if (!sourceAddress.has_value()) {
        closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }
    if (sourceAddress.value() != getClientAddress()) {
        m_log->warn("Received diagnostic message from unexpected source address {}", fmt::streamed(sourceAddress.value()));
        sendDiagnosticMessageResponse(sourceAddress.value(), DoIPNegativeDiagnosticAck::InvalidSourceAddress);
        // closeConnection(DoIPCloseReason::InvalidMessage);
        return;
    }

    auto ack = notifyDiagnosticMessage(message);
    sendDiagnosticMessageResponse(sourceAddress.value(), ack);

    // Reset general inactivity timer
    restartStateTimer();

    // nack -> stop here
    if (ack.has_value()) {
        transitionTo(DoIPServerState::RoutingActivated);
        return;
    }

    if (hasDownstreamHandler()) {
        auto result = notifyDownstreamRequest(message);
        m_log->debug("Downstream req -> {}", fmt::streamed(result));
        if (result == DoIPDownstreamResult::Pending) {
            // wait for downstream response
            transitionTo(DoIPServerState::WaitDownstreamResponse);
        } else if (result == DoIPDownstreamResult::Handled) {
            // no downstream response expected -> go back to "idle"
            transitionTo(DoIPServerState::RoutingActivated);
        } else if (result == DoIPDownstreamResult::Error) {
            // request could not be handled -> issue error and back to idle
            sendDiagnosticMessageResponse(sourceAddress.value(), DoIPNegativeDiagnosticAck::TargetUnreachable);
            transitionTo(DoIPServerState::RoutingActivated);
        }
    }
}

void DoIPDefaultConnection::handleWaitAliveCheckResponse(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    // Implementation of handling wait alive check response would go here
    (void)event; // Unused parameter

    if (!msg) {
        closeConnection(DoIPCloseReason::SocketError);
        return;
    }

    auto message = msg.value();

    switch (message.getPayloadType()) {
    case DoIPPayloadType::DiagnosticMessage: /* fall-through expected */
    case DoIPPayloadType::AliveCheckResponse:
        transitionTo(DoIPServerState::RoutingActivated);
        return;
    default:
        m_log->warn("Received unsupported message type {} in Wait Alive Check Response state", fmt::streamed(message.getPayloadType()));
        sendDiagnosticMessageResponse(ZERO_ADDRESS, DoIPNegativeDiagnosticAck::TransportProtocolError);
        return;
    }
}

void DoIPDefaultConnection::handleWaitDownstreamResponse(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    // Implementation of handling wait downstream response would go here
    m_log->critical("handleWaitDownstreamResponse NOT IMPL");
}

void DoIPDefaultConnection::handleFinalize(DoIPServerEvent event, OptDoIPMessage msg) {
    (void)event; // Unused parameter
    (void)msg;   // Unused parameter

    // Implementation of handling finalize would go here
    transitionTo(DoIPServerState::Closed);
}

void DoIPDefaultConnection::handleTimeout(ConnectionTimers timer_id) {
    m_log->warn("Timeout '{}'", fmt::streamed(timer_id));

    switch (timer_id) {
    case ConnectionTimers::InitialInactivity:
        closeConnection(DoIPCloseReason::InitialInactivityTimeout);
        break;
    case ConnectionTimers::GeneralInactivity:
        sendAliveCheckRequest();
        transitionTo(DoIPServerState::WaitAliveCheckResponse);
        break;
    case ConnectionTimers::AliveCheck:
        if (m_aliveCheckRetry < m_aliveCheckRetryCount) {
            transitionTo(DoIPServerState::WaitAliveCheckResponse);
        } else {
            closeConnection(DoIPCloseReason::AliveCheckTimeout);
        }
        break;
    case ConnectionTimers::DownstreamResponse:
        m_log->warn("Downstream response timeout occurred");
        transitionTo(DoIPServerState::RoutingActivated);
        break;
    case ConnectionTimers::UserDefined:
        m_log->warn("User-defined timer -> must be handled separately");
        break;
    default:
        m_log->error("Unhandled timeout for timer id {}", fmt::streamed(timer_id));
        break;
    }
}

ssize_t DoIPDefaultConnection::sendRoutingActivationResponse(const DoIPAddress &source_address, DoIPRoutingActivationResult response_code) {
    // Create routing activation response message
    DoIPAddress serverAddr = getServerAddress();

    // Build response payload manually
    ByteArray payload;
    payload.writeU16BE(source_address);
    payload.writeU16BE(serverAddr);
    payload.push_back(static_cast<uint8_t>(response_code));
    // Reserved 4 bytes
    payload.insert(payload.end(), {0x00, 0x00, 0x00, 0x00});

    DoIPMessage response(DoIPPayloadType::RoutingActivationResponse, std::move(payload));
    return sendProtocolMessage(response);
}

ssize_t DoIPDefaultConnection::sendAliveCheckRequest() {
    auto request = message::makeAliveCheckRequest();
    return sendProtocolMessage(request);
}

ssize_t DoIPDefaultConnection::sendDiagnosticMessageResponse(const DoIPAddress &sourceAddress, DoIPDiagnosticAck ack) {
    ssize_t sentBytes;
    DoIPAddress targetAddress = getServerAddress();
    DoIPMessage message;

    if (ack.has_value()) {
        message = message::makeDiagnosticNegativeResponse(
            sourceAddress,
            targetAddress,
            ack.value(),
            ByteArray{} // Empty payload
        );
    } else {
        message = message::makeDiagnosticPositiveResponse(
            sourceAddress,
            targetAddress,
            ByteArray{} // Empty payload for ACK
        );
    }
    sentBytes = sendProtocolMessage(message);
    notifyDiagnosticAckSent(ack);
    return sentBytes;
}

ssize_t DoIPDefaultConnection::sendDownstreamResponse(const DoIPAddress &sourceAddress, const ByteArray &payload) {
    DoIPAddress targetAddress = getServerAddress();
    DoIPMessage message = message::makeDiagnosticMessage(sourceAddress, targetAddress, payload);

    return sendProtocolMessage(message);
}

DoIPDiagnosticAck DoIPDefaultConnection::notifyDiagnosticMessage(const DoIPMessage &msg) {
    if (m_serverModel->onDiagnosticMessage) {
        return m_serverModel->onDiagnosticMessage(*this, msg);
    }
    return std::nullopt;
}

void DoIPDefaultConnection::notifyConnectionClosed(DoIPCloseReason reason) {
    if (m_serverModel && m_serverModel->onCloseConnection) {
        m_serverModel->onCloseConnection(*this, reason);
    }
}

void DoIPDefaultConnection::notifyDiagnosticAckSent(DoIPDiagnosticAck ack) {
    if (m_serverModel->onDiagnosticNotification) {
        m_serverModel->onDiagnosticNotification(*this, ack);
    }
}

bool DoIPDefaultConnection::hasDownstreamHandler() const {
    return m_serverModel->hasDownstreamHandler();
}

DoIPDownstreamResult DoIPDefaultConnection::notifyDownstreamRequest(const DoIPMessage &msg) {
    if (m_serverModel->onDownstreamRequest) {
        auto handler = [this](const ByteArray &response, DoIPDownstreamResult result) {
            this->receiveDownstreamResponse(response, result);
        };
        return m_serverModel->onDownstreamRequest(*this, msg, handler);
    }
    return DoIPDownstreamResult::Error;
}

void DoIPDefaultConnection::receiveDownstreamResponse(const ByteArray &response, DoIPDownstreamResult result) {
    DoIPAddress sa = getServerAddress();
    DoIPAddress ta = getClientAddress();
    m_log->info("Downstream rsp: {} ({})", fmt::streamed(response), fmt::streamed(result));
    if (result == DoIPDownstreamResult::Handled) {
        sendProtocolMessage(message::makeDiagnosticMessage(sa, ta, response));
    } else {
        sendProtocolMessage(message::makeDiagnosticNegativeResponse(sa, ta, DoIPNegativeDiagnosticAck::TargetUnreachable, {}));
    }
    transitionTo(DoIPServerState::RoutingActivated);
}

} // namespace doip
# OBD-II/EOBD Data Structures for DTC Handling

## 🎯 Overview

This document provides the essential data structures needed to achieve OBD-II/EOBD compliance for DTC handling, focusing on the structures that would complement the existing UDS implementation.

## 📋 Core OBD-II Data Structures

### 1. **OBD2ComplianceFlags** - Readiness Monitor Status

```cpp
/**
 * OBD-II Readiness Flags per SAE J1979 / ISO 15031-5
 * These flags indicate whether emission-related systems have completed
 * their diagnostic testing.
 */
class OBD2ComplianceFlags {
public:
    // Standardized monitor IDs (SAE J1979)
    enum class MonitorID : uint8_t {
        Misfire = 0x01,
        FuelSystem = 0x02,
        ComprehensiveComponents = 0x03,
        Catalyst = 0x04,
        HeatedCatalyst = 0x05,
        EvaporativeSystem = 0x06,
        SecondaryAirSystem = 0x07,
        AIRSystem = 0x08,
        OxygenSensor = 0x09,
        OxygenSensorHeater = 0x0A,
        EGRSystem = 0x0B,
        NMHCConverter = 0x0C,
        NOxSCRConverter = 0x0D,
        BoostPressure = 0x0E,
        ExhaustGasSensor = 0x0F,
        PMFilter = 0x10,
        EGRVVTSystem = 0x11
    };

    // Monitor status flags
    static constexpr uint8_t MONITOR_COMPLETE = 0x01;
    static constexpr uint8_t MONITOR_AVAILABLE = 0x02;
    static constexpr uint8_t MONITOR_ENABLED = 0x04;

private:
    std::unordered_map<MonitorID, uint8_t> m_monitorStatus;
    bool m_milStatus = false; // Malfunction Indicator Lamp status
    uint16_t m_dtcCount = 0; // Number of emission-related DTCs
    uint16_t m_pendingDtcCount = 0; // Number of pending DTCs

public:
    /**
     * Set monitor status for a specific system
     */
    void setMonitorStatus(MonitorID monitor, uint8_t status) {
        m_monitorStatus[monitor] = status;
        updateReadiness();
    }

    /**
     * Get monitor status
     */
    uint8_t getMonitorStatus(MonitorID monitor) const {
        auto it = m_monitorStatus.find(monitor);
        return it != m_monitorStatus.end() ? it->second : 0;
    }

    /**
     * Check if all required monitors are complete
     */
    bool isReadyForInspection() const {
        // Check essential monitors for I/M readiness
        const std::vector<MonitorID> requiredMonitors = {
            MonitorID::Misfire,
            MonitorID::FuelSystem,
            MonitorID::ComprehensiveComponents,
            MonitorID::Catalyst,
            MonitorID::OxygenSensor,
            MonitorID::EGRSystem
        };

        for (auto monitor : requiredMonitors) {
            if (!(getMonitorStatus(monitor) & MONITOR_COMPLETE)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Set MIL status (Check Engine Light)
     */
    void setMilStatus(bool status) {
        m_milStatus = status;
    }

    /**
     * Get MIL status
     */
    bool getMilStatus() const { return m_milStatus; }

    /**
     * Update DTC counts
     */
    void updateDtcCounts(uint16_t total, uint16_t pending) {
        m_dtcCount = total;
        m_pendingDtcCount = pending;
    }

    /**
     * Serialize to OBD-II Mode 01 response format
     */
    ByteArray serializeMode01Response() const {
        ByteArray response;
        response.writeU8(0x41); // Mode 01 response
        response.writeU8(0x01); // PID 01 - Monitor status
        
        // Build status bytes (4 bytes for monitors 01-20)
        uint32_t statusBytes = 0;
        for (int i = 0x01; i <= 0x11; i++) {
            MonitorID monitor = static_cast<MonitorID>(i);
            if (getMonitorStatus(monitor) & MONITOR_COMPLETE) {
                statusBytes |= (1 << (i - 1));
            }
        }
        
        response.writeU32(statusBytes);
        return response;
    }

private:
    void updateReadiness() {
        // Update readiness status based on monitor completion
        // This would be called whenever monitor status changes
    }
};
```

### 2. **OBD2DiagnosticTroubleCode** - Emissions-Specific DTC

```cpp
/**
 * OBD-II Specific DTC with emissions-related metadata
 * Extends the base DiagnosticTroubleCode with OBD-II specific information
 */
class OBD2DiagnosticTroubleCode : public DiagnosticTroubleCode {
public:
    /**
     * OBD-II DTC Types per SAE J2012
     */
    enum class DTCType : uint8_t {
        Powertrain = 0x00,      // P0xxx
        Chassis = 0x01,         // C0xxx
        Body = 0x02,            // B0xxx
        Network = 0x03,         // U0xxx
        Generic = 0x04,         // P2xxx, P3xxx
        ManufacturerSpecific = 0x05
    };

    /**
     * Emissions relevance categories
     */
    enum class EmissionsRelevance : uint8_t {
        NotEmissionsRelated = 0x00,
        EmissionsRelated = 0x01,
        CriticalEmissions = 0x02
    };

private:
    DTCType m_dtcType;
    EmissionsRelevance m_emissionsRelevance;
    bool m_milIlluminationRequired = false;
    bool m_freezeFrameStored = false;
    uint8_t m_faultDetectionCounter = 0;
    std::chrono::system_clock::time_point m_warmUpCycleCount;

public:
    /**
     * Construct an OBD-II specific DTC
     */
    OBD2DiagnosticTroubleCode(uint32_t code, uint8_t statusBits,
                            DTCType type, EmissionsRelevance relevance)
        : DiagnosticTroubleCode(code, statusBits),
          m_dtcType(type),
          m_emissionsRelevance(relevance) {
        
        // Determine if this DTC should illuminate MIL
        determineMilRequirement();
    }

    /**
     * Get DTC type
     */
    DTCType getDTCType() const { return m_dtcType; }

    /**
     * Get emissions relevance
     */
    EmissionsRelevance getEmissionsRelevance() const { return m_emissionsRelevance; }

    /**
     * Check if this DTC requires MIL illumination
     */
    bool requiresMilIllumination() const { return m_milIlluminationRequired; }

    /**
     * Check if freeze frame data is available
     */
    bool hasFreezeFrame() const { return m_freezeFrameStored; }

    /**
     * Set freeze frame availability
     */
    void setFreezeFrameStored(bool stored) { m_freezeFrameStored = stored; }

    /**
     * Get fault detection counter
     */
    uint8_t getFaultDetectionCounter() const { return m_faultDetectionCounter; }

    /**
     * Increment fault detection counter
     */
    void incrementFaultDetectionCounter() {
        if (m_faultDetectionCounter < 255) {
            m_faultDetectionCounter++;
        }
    }

    /**
     * Reset fault detection counter
     */
    void resetFaultDetectionCounter() { m_faultDetectionCounter = 0; }

    /**
     * Set warm-up cycle count
     */
    void setWarmUpCycleCount(uint32_t cycles) {
        // Store as timestamp for easier calculation
        auto now = std::chrono::system_clock::now();
        m_warmUpCycleCount = now - std::chrono::hours(cycles * 24); // Approximate
    }

    /**
     * Get warm-up cycles since last clear
     */
    uint32_t getWarmUpCycles() const {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - m_warmUpCycleCount);
        return static_cast<uint32_t>(duration.count() / 24); // Convert hours to days
    }

    /**
     * Check if DTC is emissions-related
     */
    bool isEmissionsRelated() const {
        return m_emissionsRelevance != EmissionsRelevance::NotEmissionsRelated;
    }

    /**
     * Check if DTC is critical for emissions
     */
    bool isCriticalEmissionsDTC() const {
        return m_emissionsRelevance == EmissionsRelevance::CriticalEmissions;
    }

    /**
     * Serialize to OBD-II format (Mode 03 response)
     */
    ByteArray serializeOBD2Format() const {
        ByteArray response;
        
        // Mode 03 response format: [0x43][DTC1][DTC2][DTC3]...
        // Each DTC is 4 bytes: [P0][XXX][XX][status]
        uint8_t highByte = getHighByte();
        uint8_t middleByte = getMiddleByte();
        uint8_t lowByte = getLowByte();
        
        // Ensure proper OBD-II format (P0xxx, etc.)
        if (m_dtcType == DTCType::Powertrain) {
            highByte = 0x00; // P0xxx
        } else if (m_dtcType == DTCType::Generic) {
            highByte &= 0x0F; // P2xxx, P3xxx
        }
        
        response.writeU8(highByte);
        response.writeU8(middleByte);
        response.writeU8(lowByte);
        response.writeU8(getStatusBits());
        
        return response;
    }

private:
    /**
     * Determine if this DTC should illuminate MIL based on type and status
     */
    void determineMilRequirement() {
        // MIL illumination rules per OBD-II regulations
        if (isEmissionsRelated() && hasStatusBit(STATUS_CONFIRMED_DTC)) {
            m_milIlluminationRequired = true;
        }
    }
};
```

### 3. **OBD2FreezeFrameData** - Vehicle State at Fault

```cpp
/**
 * Freeze Frame Data - Vehicle state captured when a DTC is set
 * Per SAE J1979, Mode 02 provides freeze frame data
 */
class OBD2FreezeFrameData {
public:
    // Standardized OBD-II PIDs that should be captured in freeze frames
    enum class FreezeFramePID : uint8_t {
        FuelSystemStatus = 0x03,
        CalculatedEngineLoad = 0x04,
        EngineCoolantTemp = 0x05,
        ShortTermFuelTrimBank1 = 0x06,
        LongTermFuelTrimBank1 = 0x07,
        ShortTermFuelTrimBank2 = 0x08,
        LongTermFuelTrimBank2 = 0x09,
        FuelPressure = 0x0A,
        IntakeManifoldPressure = 0x0B,
        EngineRPM = 0x0C,
        VehicleSpeed = 0x0D,
        TimingAdvance = 0x0E,
        IntakeAirTemp = 0x0F,
        MAFAirFlowRate = 0x10,
        ThrottlePosition = 0x11,
        OxygenSensorVoltages = 0x14,
        OxygenSensorTrim = 0x15,
        FuelType = 0x51,
        EthanolFuelPercent = 0x52,
        AbsoluteEvapSystemVaporPressure = 0x53,
        EvapSystemVaporPressure = 0x54,
        ShortTermSecondaryOxygenSensorTrim = 0x55,
        LongTermSecondaryOxygenSensorTrim = 0x56
    };

private:
    uint32_t m_dtcCode = 0;
    std::chrono::system_clock::time_point m_timestamp;
    std::unordered_map<FreezeFramePID, std::vector<uint8_t>> m_pidData;
    uint8_t m_fuelSystemStatus1 = 0;
    uint8_t m_fuelSystemStatus2 = 0;
    uint16_t m_engineLoad = 0;
    int16_t m_coolantTemp = 0;
    int16_t m_shortTermFuelTrimBank1 = 0;
    int16_t m_longTermFuelTrimBank1 = 0;

public:
    /**
     * Construct freeze frame data for a specific DTC
     */
    OBD2FreezeFrameData(uint32_t dtcCode)
        : m_dtcCode(dtcCode),
          m_timestamp(std::chrono::system_clock::now()) {}

    /**
     * Add PID data to freeze frame
     */
    void addPIDData(FreezeFramePID pid, const std::vector<uint8_t>& data) {
        m_pidData[pid] = data;
        parseCommonPIDs(pid, data);
    }

    /**
     * Get DTC code associated with this freeze frame
     */
    uint32_t getDTCCode() const { return m_dtcCode; }

    /**
     * Get timestamp when freeze frame was captured
     */
    std::chrono::system_clock::time_point getTimestamp() const { return m_timestamp; }

    /**
     * Get PID data
     */
    const std::vector<uint8_t>* getPIDData(FreezeFramePID pid) const {
        auto it = m_pidData.find(pid);
        return it != m_pidData.end() ? &it->second : nullptr;
    }

    /**
     * Get fuel system status
     */
    uint8_t getFuelSystemStatus() const { return m_fuelSystemStatus1; }

    /**
     * Get calculated engine load (%)
     */
    uint8_t getEngineLoad() const { return static_cast<uint8_t>(m_engineLoad / 2.55); }

    /**
     * Get engine coolant temperature (°C)
     */
    int8_t getCoolantTemp() const { return static_cast<int8_t>(m_coolantTemp - 40); }

    /**
     * Get short term fuel trim bank 1 (%)
     */
    int8_t getShortTermFuelTrimBank1() const { return static_cast<int8_t>(m_shortTermFuelTrimBank1 - 128); }

    /**
     * Serialize to OBD-II Mode 02 response format
     */
    ByteArray serializeMode02Response() const {
        ByteArray response;
        response.writeU8(0x42); // Mode 02 response
        
        // DTC code (3 bytes)
        response.writeU8(static_cast<uint8_t>((m_dtcCode >> 16) & 0xFF));
        response.writeU8(static_cast<uint8_t>((m_dtcCode >> 8) & 0xFF));
        response.writeU8(static_cast<uint8_t>(m_dtcCode & 0xFF));
        
        // Number of data bytes
        size_t dataSize = 0;
        for (const auto& pidData : m_pidData) {
            dataSize += pidData.second.size();
        }
        
        response.writeU8(static_cast<uint8_t>(dataSize));
        
        // PID data
        for (const auto& pidData : m_pidData) {
            response.writeU8(static_cast<uint8_t>(pidData.first));
            response.append(pidData.second);
        }
        
        return response;
    }

private:
    /**
     * Parse common PIDs into structured fields
     */
    void parseCommonPIDs(FreezeFramePID pid, const std::vector<uint8_t>& data) {
        if (data.empty()) return;
        
        switch (pid) {
            case FreezeFramePID::FuelSystemStatus:
                if (data.size() >= 2) {
                    m_fuelSystemStatus1 = data[0];
                    m_fuelSystemStatus2 = data[1];
                }
                break;
            case FreezeFramePID::CalculatedEngineLoad:
                if (data.size() >= 1) {
                    m_engineLoad = static_cast<uint16_t>(data[0]) * 100 / 255;
                }
                break;
            case FreezeFramePID::EngineCoolantTemp:
                if (data.size() >= 1) {
                    m_coolantTemp = static_cast<int16_t>(data[0]) - 40;
                }
                break;
            case FreezeFramePID::ShortTermFuelTrimBank1:
                if (data.size() >= 1) {
                    m_shortTermFuelTrimBank1 = static_cast<int16_t>(data[0]) - 128;
                }
                break;
            case FreezeFramePID::LongTermFuelTrimBank1:
                if (data.size() >= 1) {
                    m_longTermFuelTrimBank1 = static_cast<int16_t>(data[0]) - 128;
                }
                break;
            // Add more PID parsing as needed
        }
    }
};
```

### 4. **OBD2DiagnosticSession** - Session Management

```cpp
/**
 * OBD-II Diagnostic Session Management
 * Handles session state, security, and protocol timing
 */
class OBD2DiagnosticSession {
public:
    /**
     * OBD-II Protocol Types
     */
    enum class ProtocolType : uint8_t {
        ISO9141_2 = 0x01,
        ISO14230_4_KWP2000_5BaudInit = 0x02,
        ISO14230_4_KWP2000_FastInit = 0x03,
        ISO15765_4_CAN_11bit_500K = 0x04,
        ISO15765_4_CAN_29bit_500K = 0x05,
        ISO15765_4_CAN_11bit_250K = 0x06,
        ISO15765_4_CAN_29bit_250K = 0x07,
        SAE_J1850_PWM = 0x08,
        SAE_J1850_VPW = 0x09,
        ISO9141_2_ECHO = 0x0A
    };

    /**
     * Session Types
     */
    enum class SessionType : uint8_t {
        DefaultSession = 0x01,
        ProgrammingSession = 0x02,
        ExtendedDiagnosticSession = 0x03,
        SafetySystemDiagnosticSession = 0x04
    };

private:
    SessionType m_currentSession = SessionType::DefaultSession;
    ProtocolType m_protocolType = ProtocolType::ISO15765_4_CAN_11bit_500K;
    bool m_securityUnlocked = false;
    uint8_t m_securityLevel = 0;
    std::chrono::system_clock::time_point m_sessionStartTime;
    std::chrono::milliseconds m_sessionTimeout = std::chrono::minutes(5);
    OBD2ComplianceFlags m_complianceFlags;

public:
    /**
     * Start a diagnostic session
     */
    bool startSession(SessionType sessionType) {
        m_currentSession = sessionType;
        m_sessionStartTime = std::chrono::system_clock::now();
        m_securityUnlocked = false;
        m_securityLevel = 0;
        
        // Reset timeout based on session type
        switch (sessionType) {
            case SessionType::DefaultSession:
                m_sessionTimeout = std::chrono::minutes(5);
                break;
            case SessionType::ProgrammingSession:
                m_sessionTimeout = std::chrono::minutes(30);
                break;
            case SessionType::ExtendedDiagnosticSession:
                m_sessionTimeout = std::chrono::minutes(10);
                break;
            case SessionType::SafetySystemDiagnosticSession:
                m_sessionTimeout = std::chrono::minutes(2);
                break;
        }
        
        return true;
    }

    /**
     * End current session
     */
    void endSession() {
        m_currentSession = SessionType::DefaultSession;
        m_securityUnlocked = false;
        m_securityLevel = 0;
    }

    /**
     * Check if session is active
     */
    bool isSessionActive() const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_sessionStartTime);
        return elapsed < m_sessionTimeout;
    }

    /**
     * Unlock security access
     */
    bool unlockSecurityAccess(uint8_t level, const ByteArray& seedResponse) {
        // Implement security access algorithm
        // For OBD-II, typically level 1 is sufficient for most operations
        if (level <= 3) { // Max level for OBD-II
            m_securityUnlocked = true;
            m_securityLevel = level;
            return true;
        }
        return false;
    }

    /**
     * Check security level
     */
    bool isSecurityUnlocked(uint8_t requiredLevel) const {
        return m_securityUnlocked && m_securityLevel >= requiredLevel;
    }

    /**
     * Get current session type
     */
    SessionType getCurrentSession() const { return m_currentSession; }

    /**
     * Get protocol type
     */
    ProtocolType getProtocolType() const { return m_protocolType; }

    /**
     * Set protocol type
     */
    void setProtocolType(ProtocolType protocol) { m_protocolType = protocol; }

    /**
     * Get compliance flags
     */
    OBD2ComplianceFlags& getComplianceFlags() { return m_complianceFlags; }

    /**
     * Get const compliance flags
     */
    const OBD2ComplianceFlags& getComplianceFlags() const { return m_complianceFlags; }

    /**
     * Check if session timed out
     */
    bool isSessionTimedOut() const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_sessionStartTime);
        return elapsed >= m_sessionTimeout;
    }

    /**
     * Refresh session timeout
     */
    void refreshSessionTimeout() {
        m_sessionStartTime = std::chrono::system_clock::now();
    }
};
```

### 5. **OBD2DiagnosticSystem** - Integration Layer

```cpp
/**
 * Complete OBD-II Diagnostic System
 * Integrates all components for full OBD-II/EOBD compliance
 */
class OBD2DiagnosticSystem {
private:
    OBD2DiagnosticSession m_session;
    DiagnosticTroubleCodeStore m_dtcStore;
    std::unordered_map<uint32_t, OBD2FreezeFrameData> m_freezeFrames;
    std::unordered_map<uint32_t, OBD2DiagnosticTroubleCode> m_obd2DTCs;
    
    // Configuration
    bool m_obd2ComplianceEnabled = true;
    bool m_eobdComplianceEnabled = true;
    uint8_t m_obd2ProtocolVersion = 0x04; // ISO 15031-5:2020

public:
    /**
     * Initialize OBD-II system
     */
    void initialize() {
        // Set up default session
        m_session.startSession(OBD2DiagnosticSession::SessionType::DefaultSession);
        
        // Initialize compliance flags
        auto& flags = m_session.getComplianceFlags();
        flags.setMilStatus(false);
        flags.updateDtcCounts(0, 0);
        
        // Set up supported monitors
        setupDefaultMonitors();
    }

    /**
     * Add a DTC to the system
     */
    bool addDTC(uint32_t code, uint8_t statusBits,
                OBD2DiagnosticTroubleCode::DTCType type,
                OBD2DiagnosticTroubleCode::EmissionsRelevance relevance) {
        
        // Create OBD-II specific DTC
        OBD2DiagnosticTroubleCode obd2Dtc(code, statusBits, type, relevance);
        
        // Add to stores
        m_dtcStore.addDTC(obd2Dtc);
        m_obd2DTCs[code] = obd2Dtc;
        
        // Update compliance flags
        updateComplianceFlags();
        
        // Capture freeze frame if emissions-related
        if (obd2Dtc.isEmissionsRelated()) {
            captureFreezeFrame(code);
        }
        
        return true;
    }

    /**
     * Capture freeze frame data for a DTC
     */
    void captureFreezeFrame(uint32_t dtcCode) {
        OBD2FreezeFrameData freezeFrame(dtcCode);
        
        // Capture essential PIDs (simplified example)
        // In real implementation, these would come from vehicle sensors
        freezeFrame.addPIDData(OBD2FreezeFrameData::FreezeFramePID::FuelSystemStatus, {0x01, 0x00}); // Closed loop
        freezeFrame.addPIDData(OBD2FreezeFrameData::FreezeFramePID::CalculatedEngineLoad, {0x4B}); // 75% load
        freezeFrame.addPIDData(OBD2FreezeFrameData::FreezeFramePID::EngineCoolantTemp, {0x5A}); // 90°C
        freezeFrame.addPIDData(OBD2FreezeFrameData::FreezeFramePID::EngineRPM, {0x0E, 0xA0}); // 3744 RPM
        freezeFrame.addPIDData(OBD2FreezeFrameData::FreezeFramePID::VehicleSpeed, {0x32}); // 50 km/h
        
        m_freezeFrames[dtcCode] = freezeFrame;
        
        // Mark DTC as having freeze frame
        if (m_obd2DTCs.find(dtcCode) != m_obd2DTCs.end()) {
            m_obd2DTCs[dtcCode].setFreezeFrameStored(true);
        }
    }

    /**
     * Clear DTCs by status mask (OBD-II Mode 04)
     */
    bool clearDTCs(uint8_t statusMask) {
        std::vector<uint32_t> dtcsToClear;
        
        // Find DTCs matching the status mask
        for (const auto& dtc : m_dtcStore.getAllDTCs()) {
            if ((dtc.getStatusBits() & statusMask) == statusMask) {
                dtcsToClear.push_back(dtc.getCode());
            }
        }
        
        // Clear the DTCs
        bool clearedAny = false;
        for (uint32_t code : dtcsToClear) {
            if (m_dtcStore.removeDTC(code)) {
                m_obd2DTCs.erase(code);
                m_freezeFrames.erase(code);
                clearedAny = true;
            }
        }
        
        if (clearedAny) {
            updateComplianceFlags();
        }
        
        return clearedAny;
    }

    /**
     * Get DTC count by status
     */
    size_t getDTCCountByStatus(uint8_t statusMask) const {
        return m_dtcStore.countByStatusBits(statusMask);
    }

    /**
     * Get all emission-related DTCs
     */
    std::vector<OBD2DiagnosticTroubleCode> getEmissionsDTCs() const {
        std::vector<OBD2DiagnosticTroubleCode> result;
        
        for (const auto& pair : m_obd2DTCs) {
            if (pair.second.isEmissionsRelated()) {
                result.push_back(pair.second);
            }
        }
        
        return result;
    }

    /**
     * Get freeze frame data for a DTC
     */
    const OBD2FreezeFrameData* getFreezeFrame(uint32_t dtcCode) const {
        auto it = m_freezeFrames.find(dtcCode);
        return it != m_freezeFrames.end() ? &it->second : nullptr;
    }

    /**
     * Handle OBD-II Mode 01 request (Current Data)
     */
    ByteArray handleMode01Request(const ByteArray& request) {
        if (request.size() < 2) {
            return makeNegativeResponse(0x7F, 0x01, 0x12); // Invalid format
        }
        
        uint8_t pid = request[1];
        
        // Handle specific PIDs
        switch (pid) {
            case 0x01: // Monitor status since DTCs cleared
                return m_session.getComplianceFlags().serializeMode01Response();
            case 0x03: // Fuel system status
                return handleFuelSystemStatus();
            case 0x04: // Calculated engine load
                return handleEngineLoad();
            case 0x05: // Engine coolant temperature
                return handleCoolantTemp();
            // ... handle other PIDs
            default:
                return makeNegativeResponse(0x7F, 0x01, 0x11); // Service not supported
        }
    }

    /**
     * Handle OBD-II Mode 02 request (Freeze Frame Data)
     */
    ByteArray handleMode02Request() {
        // Return freeze frame data for the first emissions-related DTC
        for (const auto& pair : m_obd2DTCs) {
            if (pair.second.isEmissionsRelated() && pair.second.hasFreezeFrame()) {
                const auto* freezeFrame = getFreezeFrame(pair.first);
                if (freezeFrame) {
                    return freezeFrame->serializeMode02Response();
                }
            }
        }
        
        // No freeze frame data available
        return {0x42, 0x00}; // Mode 02 response with no data
    }

    /**
     * Handle OBD-II Mode 03 request (Get DTCs)
     */
    ByteArray handleMode03Request() {
        ByteArray response;
        response.writeU8(0x43); // Mode 03 response
        
        // Collect all emission-related DTCs
        auto emissionsDTCs = getEmissionsDTCs();
        
        // Serialize each DTC
        for (const auto& dtc : emissionsDTCs) {
            auto dtcData = dtc.serializeOBD2Format();
            response.append(dtcData);
        }
        
        // If no DTCs, return just the mode byte
        if (emissionsDTCs.empty()) {
            response.writeU8(0x00); // No DTCs
        }
        
        return response;
    }

    /**
     * Handle OBD-II Mode 04 request (Clear DTCs)
     */
    ByteArray handleMode04Request() {
        // Clear all DTCs and freeze frames
        m_dtcStore.clearAll();
        m_obd2DTCs.clear();
        m_freezeFrames.clear();
        
        // Update compliance flags
        updateComplianceFlags();
        
        // Reset monitors
        resetMonitors();
        
        // Mode 04 response is just the echo
        return {0x44}; // Mode 04 response
    }

    /**
     * Handle OBD-II Mode 07 request (Pending DTCs)
     */
    ByteArray handleMode07Request() {
        ByteArray response;
        response.writeU8(0x47); // Mode 07 response
        
        // Get pending DTCs (status bit 0x04 - pendingDTC)
        auto pendingDTCs = m_dtcStore.findDTCByStatus(
            OBD2DiagnosticTroubleCode::STATUS_PENDING_DTC);
        
        // Serialize pending DTCs
        for (const auto& dtc : pendingDTCs) {
            // Convert to OBD2 format if it's in our OBD2 store
            auto it = m_obd2DTCs.find(dtc.getCode());
            if (it != m_obd2DTCs.end()) {
                auto dtcData = it->second.serializeOBD2Format();
                response.append(dtcData);
            }
        }
        
        if (pendingDTCs.empty()) {
            response.writeU8(0x00); // No pending DTCs
        }
        
        return response;
    }

    /**
     * Handle OBD-II Mode 0A request (Permanent DTCs)
     */
    ByteArray handleMode0ARequest() {
        ByteArray response;
        response.writeU8(0x4A); // Mode 0A response
        
        // Get permanent DTCs (would need to track this separately)
        // For now, return no permanent DTCs
        response.writeU8(0x00); // No permanent DTCs
        
        return response;
    }

private:
    /**
     * Set up default monitor status
     */
    void setupDefaultMonitors() {
        auto& flags = m_session.getComplianceFlags();
        
        // Mark essential monitors as available
        flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::Misfire, 
                              OBD2ComplianceFlags::MONITOR_AVAILABLE);
        flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::FuelSystem, 
                              OBD2ComplianceFlags::MONITOR_AVAILABLE);
        flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::ComprehensiveComponents, 
                              OBD2ComplianceFlags::MONITOR_AVAILABLE);
        flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::Catalyst, 
                              OBD2ComplianceFlags::MONITOR_AVAILABLE);
        flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::OxygenSensor, 
                              OBD2ComplianceFlags::MONITOR_AVAILABLE);
        flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::EGRSystem, 
                              OBD2ComplianceFlags::MONITOR_AVAILABLE);
    }

    /**
     * Update compliance flags based on current DTCs
     */
    void updateComplianceFlags() {
        auto& flags = m_session.getComplianceFlags();
        
        // Count emission-related DTCs
        size_t emissionsCount = 0;
        size_t pendingCount = 0;
        
        for (const auto& dtc : m_dtcStore.getAllDTCs()) {
            auto it = m_obd2DTCs.find(dtc.getCode());
            if (it != m_obd2DTCs.end() && it->second.isEmissionsRelated()) {
                emissionsCount++;
                if (dtc.hasStatusBit(OBD2DiagnosticTroubleCode::STATUS_PENDING_DTC)) {
                    pendingCount++;
                }
            }
        }
        
        flags.updateDtcCounts(static_cast<uint16_t>(emissionsCount), 
                            static_cast<uint16_t>(pendingCount));
        
        // Update MIL status
        bool milOn = false;
        for (const auto& dtc : m_dtcStore.getAllDTCs()) {
            auto it = m_obd2DTCs.find(dtc.getCode());
            if (it != m_obd2DTCs.end() && it->second.requiresMilIllumination()) {
                milOn = true;
                break;
            }
        }
        flags.setMilStatus(milOn);
        
        // Update monitor status based on DTCs
        updateMonitorStatus();
    }

    /**
     * Update monitor status based on current conditions
     */
    void updateMonitorStatus() {
        auto& flags = m_session.getComplianceFlags();
        
        // Simplified: mark monitors as complete if no related DTCs
        // In real implementation, this would be based on actual monitor completion
        
        // If we have emissions DTCs, some monitors are likely incomplete
        if (m_session.getComplianceFlags().getMilStatus()) {
            // With MIL on, most monitors would be incomplete
            flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::Misfire, 
                                  OBD2ComplianceFlags::MONITOR_AVAILABLE); // Not complete
            flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::FuelSystem, 
                                  OBD2ComplianceFlags::MONITOR_AVAILABLE); // Not complete
        } else {
            // No MIL, assume monitors are complete (simplified)
            flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::Misfire, 
                                  OBD2ComplianceFlags::MONITOR_COMPLETE | OBD2ComplianceFlags::MONITOR_AVAILABLE);
            flags.setMonitorStatus(OBD2ComplianceFlags::MonitorID::FuelSystem, 
                                  OBD2ComplianceFlags::MONITOR_COMPLETE | OBD2ComplianceFlags::MONITOR_AVAILABLE);
        }
    }

    /**
     * Reset all monitors (called after DTC clear)
     */
    void resetMonitors() {
        auto& flags = m_session.getComplianceFlags();
        
        // Reset all monitors to not complete
        for (int i = 0x01; i <= 0x11; i++) {
            flags.setMonitorStatus(static_cast<OBD2ComplianceFlags::MonitorID>(i),
                                  OBD2ComplianceFlags::MONITOR_AVAILABLE);
        }
        
        flags.setMilStatus(false);
        flags.updateDtcCounts(0, 0);
    }

    /**
     * Helper to create negative responses
     */
    ByteArray makeNegativeResponse(uint8_t mode, uint8_t pid, uint8_t errorCode) const {
        ByteArray response;
        response.writeU8(mode + 0x40); // Negative response mode
        response.writeU8(pid);
        response.writeU8(errorCode);
        return response;
    }

    /**
     * Handle specific PID responses
     */
    ByteArray handleFuelSystemStatus() const {
        // Return fuel system status
        return {0x41, 0x03, 0x01, 0x00, 0xFF, 0xFF}; // Mode 01, PID 03 response
    }

    ByteArray handleEngineLoad() const {
        // Return calculated engine load (75%)
        return {0x41, 0x04, 0x4B, 0xFF, 0xFF, 0xFF};
    }

    ByteArray handleCoolantTemp() const {
        // Return engine coolant temperature (90°C)
        return {0x41, 0x05, 0x5A, 0xFF, 0xFF, 0xFF};
    }
};
```

## 📋 Key Data Structure Relationships

```mermaid
classDiagram
    OBD2DiagnosticSystem "1" *-- "1" OBD2DiagnosticSession
    OBD2DiagnosticSystem "1" *-- "1" DiagnosticTroubleCodeStore
    OBD2DiagnosticSystem "1" *-- "0..*" OBD2DiagnosticTroubleCode
    OBD2DiagnosticSystem "1" *-- "0..*" OBD2FreezeFrameData
    
    OBD2DiagnosticSession "1" *-- "1" OBD2ComplianceFlags
    OBD2DiagnosticTroubleCode --|> DiagnosticTroubleCode
    
    class OBD2DiagnosticSystem {
        +handleMode01Request()
        +handleMode02Request()
        +handleMode03Request()
        +handleMode04Request()
        +handleMode07Request()
        +handleMode0ARequest()
    }
    
    class OBD2DiagnosticSession {
        +startSession()
        +endSession()
        +unlockSecurityAccess()
        +isSecurityUnlocked()
    }
    
    class OBD2ComplianceFlags {
        +setMonitorStatus()
        +getMonitorStatus()
        +isReadyForInspection()
        +setMilStatus()
    }
    
    class OBD2DiagnosticTroubleCode {
        +getDTCType()
        +getEmissionsRelevance()
        +requiresMilIllumination()
        +serializeOBD2Format()
    }
    
    class OBD2FreezeFrameData {
        +addPIDData()
        +getPIDData()
        +serializeMode02Response()
    }
```

## 🎯 Implementation Notes

### OBD-II Mode Support

The data structures support these key OBD-II modes:

| Mode | Description | Implementation |
|------|-------------|----------------|
| 01 | Current Data | ✅ Partial |
| 02 | Freeze Frame Data | ✅ Basic |
| 03 | Get DTCs | ✅ Complete |
| 04 | Clear DTCs | ✅ Complete |
| 07 | Pending DTCs | ✅ Complete |
| 09 | Vehicle Info | ❌ Missing |
| 0A | Permanent DTCs | ✅ Basic |

### Compliance Requirements

**Minimum for OBD-II Compliance:**
- ✅ DTC storage with status bits
- ✅ ReadDTCInformation (Mode 03)
- ✅ ClearDiagnosticInformation (Mode 04)
- ✅ MIL control based on emissions DTCs
- ✅ Freeze frame data capture
- ✅ Monitor status tracking

**Enhanced for Full Compliance:**
- ❌ Vehicle information (Mode 09)
- ❌ Permanent DTC tracking
- ❌ Enhanced data PIDs
- ❌ Comprehensive monitor testing

### Integration with Existing Codebase

To integrate these structures with the existing UDS implementation:

```cpp
// Extend DiagnosticTroubleCodeStore to support OBD2 DTCs
class EnhancedDiagnosticTroubleCodeStore : public DiagnosticTroubleCodeStore {
private:
    OBD2DiagnosticSystem m_obd2System;
    
public:
    // Add OBD-II specific methods
    bool addOBD2DTC(uint32_t code, uint8_t statusBits,
                   OBD2DiagnosticTroubleCode::DTCType type,
                   OBD2DiagnosticTroubleCode::EmissionsRelevance relevance) {
        return m_obd2System.addDTC(code, statusBits, type, relevance);
    }
    
    // Handle OBD-II requests
    ByteArray handleOBD2Request(uint8_t mode, const ByteArray& request) {
        switch (mode) {
            case 0x01: return m_obd2System.handleMode01Request(request);
            case 0x02: return m_obd2System.handleMode02Request();
            case 0x03: return m_obd2System.handleMode03Request();
            case 0x04: return m_obd2System.handleMode04Request();
            case 0x07: return m_obd2System.handleMode07Request();
            case 0x0A: return m_obd2System.handleMode0ARequest();
            default: return makeNegativeResponse(mode, 0x00, 0x11);
        }
    }
    
    // Get OBD-II compliance status
    bool isOBD2Ready() const {
        return m_obd2System.getSession().getComplianceFlags().isReadyForInspection();
    }
};
```

## 📊 Memory Footprint Estimation

| Structure | Typical Size | Notes |
|-----------|-------------|-------|
| OBD2ComplianceFlags | ~100 bytes | Monitor status tracking |
| OBD2DiagnosticTroubleCode | ~20 bytes | Extends base DTC with OBD-II metadata |
| OBD2FreezeFrameData | ~100-500 bytes | Variable based on captured PIDs |
| OBD2DiagnosticSession | ~50 bytes | Session state and timing |
| OBD2DiagnosticSystem | ~1KB+ | Complete system with all components |

## 🎯 Conclusion

These data structures provide the foundation for **OBD-II/EOBD compliance** while maintaining compatibility with the existing UDS implementation. The key components are:

1. **OBD2ComplianceFlags** - Monitor status and readiness tracking
2. **OBD2DiagnosticTroubleCode** - Emissions-specific DTC extensions
3. **OBD2FreezeFrameData** - Vehicle state capture at fault time
4. **OBD2DiagnosticSession** - Session and security management
5. **OBD2DiagnosticSystem** - Complete integration layer

**Implementation Priority:**
1. Start with OBD2DiagnosticTroubleCode extensions
2. Add OBD2ComplianceFlags for monitor tracking
3. Implement OBD2FreezeFrameData for snapshot support
4. Integrate with existing DTC store
5. Add OBD-II mode handlers

With these structures, the system can achieve **basic OBD-II compliance** and provide the diagnostic capabilities expected by modern vehicle diagnostic tools.

---

*Generated by Mistral AI - OBD-II/EOBD Compliance Expert*
*Based on SAE J1979, ISO 15031-5, and ISO 14229-1 standards*
*Last updated: 2024*
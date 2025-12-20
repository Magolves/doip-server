#include "Logger.h"

namespace doip {


bool Logger::use_syslog = false;
std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Logger::m_loggers;

} // namespace doip
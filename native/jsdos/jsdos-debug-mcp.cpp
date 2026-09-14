//
// Created by caiiiycuk on 14.09.2026.
//

#include "src/debug/debug_mcp.h"

void ControlServer_Start(uint16_t port) {
}
void ControlServer_Stop() {
}
bool ControlServer_IsConnected() {
  return false;
}
void ControlServer_Send(std::string message) {
}
void ControlServer_SendEvent(
        const std::string& event,
        const std::string& data) {
}
void ControlServer_Poll() {
  return;
}
bool DEBUG_MCP_IsCapturingOutput() {
  return false;
}
void DEBUG_MCP_CaptureMessage(const char* message) {
}


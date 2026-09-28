#include "src/debug/debug_mcp.h"

#include "config.h"
#include "debug.h"

#include <emscripten.h>
#include <emscripten/websocket.h>

#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace {

constexpr const char* CONTROL_HOST = "127.0.0.1";

constexpr int RECONNECT_DELAY_MS = 1000;

constexpr size_t MAX_INCOMING_MESSAGES = 1024;
constexpr size_t MAX_OUTGOING_MESSAGES = 1024;
constexpr size_t MAX_CAPTURE_LINES = 16384;
constexpr size_t MAX_RECEIVE_BUFFER = 1024 * 1024;

bool g_running = false;
bool g_connected = false;
bool g_reconnect_scheduled = false;

uint16_t g_port = 0;
EMSCRIPTEN_WEBSOCKET_T g_socket = 0;

std::deque<std::string> g_incoming;
std::deque<std::string> g_outgoing;

bool g_capture_active = false;
size_t g_capture_discarded = 0;
std::vector<std::string> g_capture_lines;

std::string g_receive_buffer;

struct ControlRequest {
    std::string id;
    std::string command;
    std::string payload;
};

bool IsSpace(const char c)
{
    return c == ' ' || c == '\t';
}

std::string TrimLeft(std::string value)
{
    while (!value.empty() && IsSpace(value.front()))
        value.erase(value.begin());

    return value;
}

std::string UpperAscii(std::string value)
{
    for (auto& c : value) {
        if (c >= 'a' && c <= 'z')
            c = static_cast<char>(c - ('a' - 'A'));
    }

    return value;
}

std::string SanitizeProtocolLine(std::string line)
{
    for (auto& c : line) {
        if (c == '\r' || c == '\n')
            c = ' ';
    }

    return line;
}

std::string FirstWordUpper(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t");
    if (begin == std::string::npos)
        return {};

    const auto end = value.find_first_of(" \t", begin);
    if (end == std::string::npos)
        return UpperAscii(value.substr(begin));

    return UpperAscii(value.substr(begin, end - begin));
}

bool IsResumeDebuggerCommand(const std::string& command)
{
    const auto word = FirstWordUpper(command);
    return word == "RUN" || word == "RUNWATCH" || word == "VRT";
}

void BeginCapture()
{
    g_capture_lines.clear();
    g_capture_discarded = 0;
    g_capture_active = true;
}

std::vector<std::string> EndCapture()
{
    g_capture_active = false;

    std::vector<std::string> lines;
    lines.swap(g_capture_lines);

    if (g_capture_discarded > 0) {
        char message[128];
        std::snprintf(message,
                      sizeof(message),
                      "[%lu debug output lines discarded]",
                      static_cast<unsigned long>(g_capture_discarded));
        lines.emplace_back(message);
        g_capture_discarded = 0;
    }

    return lines;
}

void QueueIncoming(std::string message)
{
    if (g_incoming.size() >= MAX_INCOMING_MESSAGES)
        g_incoming.pop_front();

    g_incoming.emplace_back(std::move(message));
}

bool PopIncoming(std::string& message)
{
    if (g_incoming.empty())
        return false;

    message = std::move(g_incoming.front());
    g_incoming.pop_front();
    return true;
}

void ProcessReceiveBuffer()
{
    for (;;) {
        const auto newline = g_receive_buffer.find('\n');

        if (newline == std::string::npos)
            break;

        std::string line = g_receive_buffer.substr(0, newline);
        g_receive_buffer.erase(0, newline + 1);

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (!line.empty())
            QueueIncoming(std::move(line));
    }
}

void ScheduleReconnect();

void CloseSocket(const bool reconnect)
{
    g_connected = false;
    g_receive_buffer.clear();

    const auto socket = g_socket;
    g_socket = 0;

    if (socket > 0) {
        emscripten_websocket_close(socket, 1000, "");
        emscripten_websocket_delete(socket);
    }

    if (reconnect)
        ScheduleReconnect();
}

bool FlushOutgoing()
{
    if (!g_connected || g_socket <= 0)
        return true;

    while (!g_outgoing.empty()) {
        const auto result = emscripten_websocket_send_utf8_text(
                g_socket,
                g_outgoing.front().c_str());

        if (result != EMSCRIPTEN_RESULT_SUCCESS)
            return false;

        g_outgoing.pop_front();
    }

    return true;
}

bool OnOpen(int,
            const EmscriptenWebSocketOpenEvent* event,
            void*)
{
    if (!g_running || event->socket != g_socket)
        return false;

    g_connected = true;
    std::fprintf(stderr,
                 "[ControlClient] connected to ws://%s:%u/\n",
                 CONTROL_HOST,
                 static_cast<unsigned>(g_port));

    if (!FlushOutgoing()) {
        std::fprintf(stderr,
                     "[ControlClient] connection lost while sending\n");
        CloseSocket(true);
    }

    return false;
}

bool OnError(int,
             const EmscriptenWebSocketErrorEvent* event,
             void*)
{
    if (event->socket != g_socket)
        return false;

    std::fprintf(stderr, "[ControlClient] WebSocket error\n");
    CloseSocket(true);
    return false;
}

bool OnClose(int,
             const EmscriptenWebSocketCloseEvent* event,
             void*)
{
    if (event->socket != g_socket)
        return false;

    std::fprintf(stderr,
                 "[ControlClient] connection closed (code %u)\n",
                 static_cast<unsigned>(event->code));

    g_connected = false;
    g_receive_buffer.clear();
    g_socket = 0;
    emscripten_websocket_delete(event->socket);
    ScheduleReconnect();
    return false;
}

bool OnMessage(int,
               const EmscriptenWebSocketMessageEvent* event,
               void*)
{
    if (!g_running || event->socket != g_socket)
        return false;

    if (!event->isText) {
        std::fprintf(stderr,
                     "[ControlClient] binary WebSocket message rejected\n");
        CloseSocket(true);
        return false;
    }

    size_t message_size = event->numBytes;
    // Emscripten includes the trailing UTF-8 null terminator in numBytes.
    if (message_size > 0 && event->data[message_size - 1] == '\0')
        --message_size;

    if (g_receive_buffer.size() + message_size > MAX_RECEIVE_BUFFER) {
        std::fprintf(stderr,
                     "[ControlClient] receive buffer overflow\n");
        CloseSocket(true);
        return false;
    }

    g_receive_buffer.append(
            reinterpret_cast<const char*>(event->data),
            message_size);
    ProcessReceiveBuffer();
    return false;
}

bool Connect()
{
    if (!g_running || g_socket > 0)
        return false;

    const auto url = std::string("ws://") + CONTROL_HOST + ":" +
                     std::to_string(g_port) + "/";

    std::fprintf(stderr,
                 "[ControlClient] connecting to %s...\n",
                 url.c_str());

    EmscriptenWebSocketCreateAttributes attributes;
    emscripten_websocket_init_create_attributes(&attributes);
    attributes.url = url.c_str();
    attributes.createOnMainThread = false;

    const auto socket = emscripten_websocket_new(&attributes);
    if (socket <= 0) {
        std::fprintf(stderr,
                     "[ControlClient] WebSocket creation failed: %d\n",
                     socket);
        ScheduleReconnect();
        return false;
    }

    g_socket = socket;

    if (emscripten_websocket_set_onopen_callback(socket, nullptr, OnOpen) !=
                    EMSCRIPTEN_RESULT_SUCCESS ||
            emscripten_websocket_set_onerror_callback(
                    socket, nullptr, OnError) != EMSCRIPTEN_RESULT_SUCCESS ||
            emscripten_websocket_set_onclose_callback(
                    socket, nullptr, OnClose) != EMSCRIPTEN_RESULT_SUCCESS ||
            emscripten_websocket_set_onmessage_callback(
                    socket, nullptr, OnMessage) != EMSCRIPTEN_RESULT_SUCCESS) {
        std::fprintf(stderr,
                     "[ControlClient] WebSocket callback registration failed\n");
        CloseSocket(true);
        return false;
    }

    return true;
}

void Reconnect(void*)
{
    g_reconnect_scheduled = false;

    if (g_running && g_socket <= 0)
        Connect();
}

void ScheduleReconnect()
{
    if (!g_running || g_reconnect_scheduled)
        return;

    g_reconnect_scheduled = true;
    emscripten_async_call(Reconnect, nullptr, RECONNECT_DELAY_MS);
}

bool ParseControlRequest(const std::string& line,
                         ControlRequest& request,
                         std::string& error)
{
    if (line.compare(0, 4, "REQ ") != 0) {
        error = "expected REQ <id> <PING|BREAK|EXEC>";
        return false;
    }

    const auto id_begin = line.find_first_not_of(" \t", 4);
    if (id_begin == std::string::npos) {
        error = "missing request id";
        return false;
    }

    const auto id_end = line.find_first_of(" \t", id_begin);
    if (id_end == std::string::npos) {
        error = "missing request command";
        return false;
    }

    request.id = line.substr(id_begin, id_end - id_begin);

    const auto command_begin = line.find_first_not_of(" \t", id_end);
    if (command_begin == std::string::npos) {
        error = "missing request command";
        return false;
    }

    const auto command_end = line.find_first_of(" \t", command_begin);
    if (command_end == std::string::npos) {
        request.command = UpperAscii(line.substr(command_begin));
        request.payload.clear();
    } else {
        request.command = UpperAscii(
                line.substr(command_begin, command_end - command_begin));
        request.payload = TrimLeft(line.substr(command_end));
    }

    if (request.command != "PING" &&
            request.command != "BREAK" &&
            request.command != "EXEC") {
        error = "unknown request command";
        return false;
    }

    if (request.command == "EXEC" && request.payload.empty()) {
        error = "missing debugger command";
        return false;
    }

    return true;
}

void SendResponse(const std::string& id,
                  const bool ok,
                  const std::vector<std::string>& lines)
{
    std::string response = std::string("BEGIN ") + id + (ok ? " OK" : " ERR");

    for (const auto& line : lines) {
        response.push_back('\n');
        response += SanitizeProtocolLine(line);
    }

    response += "\nEND ";
    response += id;

    ControlServer_Send(std::move(response));
}

void SendErrorResponse(const std::string& id, const std::string& error)
{
    SendResponse(id.empty() ? "0" : id, false, {error});
}

void ProcessControlCommand(const std::string& line)
{
    ControlRequest request;
    std::string error;

    if (!ParseControlRequest(line, request, error)) {
        SendErrorResponse(request.id, error);
        return;
    }

    if (request.command == "PING") {
        SendResponse(request.id, true, {"PONG"});
        return;
    }

    if (request.command == "EXEC" &&
            IsResumeDebuggerCommand(request.payload)) {
        SendResponse(request.id, true, {"OK"});
        DEBUG_ExecuteCommand(request.payload.c_str());
        return;
    }

    BeginCapture();

    bool ok = true;
    if (request.command == "BREAK") {
        DEBUG_EnableDebugger();
    } else {
        ok = DEBUG_ExecuteCommand(request.payload.c_str());
    }

    auto output = EndCapture();
    SendResponse(request.id, ok, output);
}

} // namespace

void ControlServer_Start(const uint16_t port)
{
    if (port == 0 || g_running)
        return;

    if (!emscripten_websocket_is_supported()) {
        std::fprintf(stderr,
                     "[ControlClient] WebSockets are not supported\n");
        return;
    }

    g_port = port;
    g_incoming.clear();
    g_outgoing.clear();
    g_receive_buffer.clear();
    g_running = true;

    Connect();
}

void ControlServer_Stop()
{
    if (!g_running)
        return;

    g_running = false;
    CloseSocket(false);
    g_port = 0;
    g_incoming.clear();
    g_outgoing.clear();
    EndCapture();
}

bool ControlServer_IsConnected()
{
    return g_connected;
}

void ControlServer_Send(std::string message)
{
    if (!g_running)
        return;

    message.push_back('\n');

    if (g_outgoing.size() >= MAX_OUTGOING_MESSAGES)
        g_outgoing.pop_front();

    g_outgoing.emplace_back(std::move(message));

    if (!FlushOutgoing()) {
        std::fprintf(stderr,
                     "[ControlClient] connection lost while sending\n");
        CloseSocket(true);
    }
}

void ControlServer_SendEvent(
        const std::string& event,
        const std::string& data)
{
    ControlServer_Send(std::string("BEGIN event OK\n") +
                       SanitizeProtocolLine(event) + " " +
                       SanitizeProtocolLine(data) +
                       "\nEND event");
}

void ControlServer_Poll()
{
    std::string command;
    while (PopIncoming(command)) {
        std::fprintf(stderr,
                     "[ControlClient] mcp command %s\n",
                     command.c_str());

        ProcessControlCommand(command);
    }
}

bool DEBUG_MCP_IsCapturingOutput()
{
    return g_capture_active;
}

void DEBUG_MCP_CaptureMessage(const char* message)
{
    if (!g_capture_active)
        return;

    if (g_capture_lines.size() >= MAX_CAPTURE_LINES) {
        ++g_capture_discarded;
        return;
    }

    g_capture_lines.emplace_back(message ? message : "");
}

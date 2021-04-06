#include <framework/global.h>
#include <framework/core/clock.h>

#include "packet_recorder.h"

PacketRecorder::PacketRecorder(const std::string& file)
{
    std::error_code ec;
    std::filesystem::create_directory("records", ec);
    m_start = g_clock.millis();
    m_stream = std::ofstream(std::filesystem::path("records") / file);
}

PacketRecorder::~PacketRecorder()
{

}

void PacketRecorder::addInputPacket(const InputMessagePtr& packet)
{
    m_stream << "< " << (g_clock.millis() - m_start) << " ";
    for (auto& buffer : packet->getBodyBuffer()) {
        m_stream << std::setfill('0') << std::setw(2) << std::hex << (uint16_t)(uint8_t)buffer;
    }
    m_stream << std::dec << "\n";
}

void PacketRecorder::addOutputPacket(const OutputMessagePtr& packet)
{
    if (m_firstOutput) {
        // skip packet with login and password
        m_firstOutput = false;
        return;
    }

    m_stream << "> " << (g_clock.millis() - m_start) << " ";
    for (auto& buffer : packet->getBuffer()) {
        m_stream << std::setfill('0') << std::setw(2) << std::hex << (uint16_t)(uint8_t)buffer;
    }
    m_stream << std::dec << "\n";
}

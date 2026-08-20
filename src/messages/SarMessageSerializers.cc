#include "messages/PositionUpdate_m.h"
#include "messages/VictimAck_m.h"
#include "messages/VictimAlert_m.h"

#include <cstring>
#include <string>

#include "inet/common/packet/serializer/ChunkSerializer.h"
#include "inet/common/packet/serializer/ChunkSerializerRegistry.h"

namespace echosar {

// Wire format: "ECHO" | version | type | body length | typed fields | padding.
// Integers and IEEE-754 doubles use network byte order (big endian).
static constexpr uint8_t WIRE_VERSION = 1;
static constexpr uint8_t POSITION_UPDATE_TYPE = 1;
static constexpr uint8_t VICTIM_ALERT_TYPE = 2;
static constexpr uint8_t VICTIM_ACK_TYPE = 3;

static void writeString(inet::MemoryOutputStream& stream, const char *value)
{
    std::string text = value == nullptr ? "" : value;
    if (text.size() > UINT16_MAX)
        throw omnetpp::cRuntimeError("ECHOSAR wire string is too long");
    stream.writeUint16Be(static_cast<uint16_t>(text.size()));
    stream.writeBytes(std::vector<uint8_t>(text.begin(), text.end()));
}

static std::string readString(inet::MemoryInputStream& stream)
{
    uint16_t length = stream.readUint16Be();
    std::string text;
    text.reserve(length);
    for (uint16_t i = 0; i < length; ++i)
        text.push_back(static_cast<char>(stream.readByte()));
    return text;
}

static void writeDouble(inet::MemoryOutputStream& stream, double value)
{
    static_assert(sizeof(double) == sizeof(uint64_t), "64-bit double required");
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    stream.writeUint64Be(bits);
}

static double readDouble(inet::MemoryInputStream& stream)
{
    uint64_t bits = stream.readUint64Be();
    double value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

static void writeTime(inet::MemoryOutputStream& stream, omnetpp::simtime_t value)
{
    writeDouble(stream, value.dbl());
}

static omnetpp::simtime_t readTime(inet::MemoryInputStream& stream)
{
    return omnetpp::simtime_t(readDouble(stream));
}

template<typename T>
struct SarCodec;

template<>
struct SarCodec<PositionUpdateChunk>
{
    static constexpr uint8_t TYPE = POSITION_UPDATE_TYPE;

    static void encode(inet::MemoryOutputStream& stream,
            const inet::Ptr<const PositionUpdateChunk>& chunk)
    {
        writeString(stream, chunk->getMessageId());
        writeString(stream, chunk->getSenderId());
        writeString(stream, chunk->getSenderType());
        writeString(stream, chunk->getIpAddress());
        stream.writeUint32Be(static_cast<uint32_t>(chunk->getWaypointId()));
        writeDouble(stream, chunk->getPositionX());
        writeDouble(stream, chunk->getPositionY());
        writeDouble(stream, chunk->getPositionZ());
        stream.writeUint64Be(static_cast<uint64_t>(chunk->getSequenceNumber()));
        writeTime(stream, chunk->getTimestamp());
        writeString(stream, chunk->getOperationalState());
    }

    static void decode(inet::MemoryInputStream& stream,
            const inet::Ptr<PositionUpdateChunk>& chunk)
    {
        chunk->setMessageId(readString(stream).c_str());
        chunk->setSenderId(readString(stream).c_str());
        chunk->setSenderType(readString(stream).c_str());
        chunk->setIpAddress(readString(stream).c_str());
        chunk->setWaypointId(static_cast<int32_t>(stream.readUint32Be()));
        chunk->setPositionX(readDouble(stream));
        chunk->setPositionY(readDouble(stream));
        chunk->setPositionZ(readDouble(stream));
        chunk->setSequenceNumber(static_cast<int64_t>(stream.readUint64Be()));
        chunk->setTimestamp(readTime(stream));
        chunk->setOperationalState(readString(stream).c_str());
    }
};

template<>
struct SarCodec<VictimAlertChunk>
{
    static constexpr uint8_t TYPE = VICTIM_ALERT_TYPE;

    static void encode(inet::MemoryOutputStream& stream,
            const inet::Ptr<const VictimAlertChunk>& chunk)
    {
        writeString(stream, chunk->getAlertId());
        writeString(stream, chunk->getMessageId());
        writeString(stream, chunk->getVictimId());
        writeString(stream, chunk->getOriginDroneId());
        writeString(stream, chunk->getOriginDroneAddress());
        writeDouble(stream, chunk->getVictimPositionX());
        writeDouble(stream, chunk->getVictimPositionY());
        writeDouble(stream, chunk->getVictimPositionZ());
        writeDouble(stream, chunk->getDronePositionX());
        writeDouble(stream, chunk->getDronePositionY());
        writeDouble(stream, chunk->getDronePositionZ());
        stream.writeUint32Be(static_cast<uint32_t>(chunk->getWaypointId()));
        stream.writeUint64Be(static_cast<uint64_t>(chunk->getSequenceNumber()));
        stream.writeUint32Be(static_cast<uint32_t>(chunk->getAttemptNumber()));
        writeTime(stream, chunk->getGenerationTimestamp());
        writeTime(stream, chunk->getTransmissionTimestamp());
        writeTime(stream, chunk->getTimeToLive());
    }

    static void decode(inet::MemoryInputStream& stream,
            const inet::Ptr<VictimAlertChunk>& chunk)
    {
        chunk->setAlertId(readString(stream).c_str());
        chunk->setMessageId(readString(stream).c_str());
        chunk->setVictimId(readString(stream).c_str());
        chunk->setOriginDroneId(readString(stream).c_str());
        chunk->setOriginDroneAddress(readString(stream).c_str());
        chunk->setVictimPositionX(readDouble(stream));
        chunk->setVictimPositionY(readDouble(stream));
        chunk->setVictimPositionZ(readDouble(stream));
        chunk->setDronePositionX(readDouble(stream));
        chunk->setDronePositionY(readDouble(stream));
        chunk->setDronePositionZ(readDouble(stream));
        chunk->setWaypointId(static_cast<int32_t>(stream.readUint32Be()));
        chunk->setSequenceNumber(static_cast<int64_t>(stream.readUint64Be()));
        chunk->setAttemptNumber(static_cast<int32_t>(stream.readUint32Be()));
        chunk->setGenerationTimestamp(readTime(stream));
        chunk->setTransmissionTimestamp(readTime(stream));
        chunk->setTimeToLive(readTime(stream));
    }
};

template<>
struct SarCodec<VictimAckChunk>
{
    static constexpr uint8_t TYPE = VICTIM_ACK_TYPE;

    static void encode(inet::MemoryOutputStream& stream,
            const inet::Ptr<const VictimAckChunk>& chunk)
    {
        writeString(stream, chunk->getAlertId());
        writeString(stream, chunk->getReceivedMessageId());
        writeString(stream, chunk->getVictimId());
        writeString(stream, chunk->getTeamId());
        writeString(stream, chunk->getOriginDroneId());
        writeTime(stream, chunk->getReceptionTimestamp());
        writeTime(stream, chunk->getAckTimestamp());
    }

    static void decode(inet::MemoryInputStream& stream,
            const inet::Ptr<VictimAckChunk>& chunk)
    {
        chunk->setAlertId(readString(stream).c_str());
        chunk->setReceivedMessageId(readString(stream).c_str());
        chunk->setVictimId(readString(stream).c_str());
        chunk->setTeamId(readString(stream).c_str());
        chunk->setOriginDroneId(readString(stream).c_str());
        chunk->setReceptionTimestamp(readTime(stream));
        chunk->setAckTimestamp(readTime(stream));
    }
};

template<typename T>
class SarChunkSerializer : public inet::ChunkSerializer
{
  public:
    virtual void serialize(inet::MemoryOutputStream& stream,
            const inet::Ptr<const inet::Chunk>& chunk, inet::b offset,
            inet::b length) const override
    {
        auto typedChunk = inet::staticPtrCast<const T>(chunk);
        inet::MemoryOutputStream body;
        SarCodec<T>::encode(body, typedChunk);
        size_t bodyLength = inet::B(body.getLength()).get();
        if (bodyLength > UINT16_MAX)
            throw omnetpp::cRuntimeError("ECHOSAR encoded body is too long");

        inet::MemoryOutputStream encoded;
        encoded.writeBytes({'E', 'C', 'H', 'O'});
        encoded.writeUint8(WIRE_VERSION);
        encoded.writeUint8(SarCodec<T>::TYPE);
        encoded.writeUint16Be(static_cast<uint16_t>(bodyLength));
        encoded.writeBytes(body.getData());

        size_t payloadLength = inet::B(chunk->getChunkLength()).get();
        size_t encodedLength = inet::B(encoded.getLength()).get();
        if (encodedLength > payloadLength)
            throw omnetpp::cRuntimeError(
                    "ECHOSAR type %u needs %zu bytes but payload is %zu bytes",
                    SarCodec<T>::TYPE, encodedLength, payloadLength);
        encoded.writeByteRepeatedly(0, payloadLength - encodedLength);

        inet::B byteOffset(offset);
        inet::B byteLength = length == inet::b(-1)
                ? inet::B(chunk->getChunkLength() - offset) : inet::B(length);
        stream.writeBytes(encoded.getData(), byteOffset, byteLength);
        totalSerializedLength += byteLength;
    }

    virtual const inet::Ptr<inet::Chunk> deserialize(
            inet::MemoryInputStream& stream,
            const std::type_info& typeInfo) const override
    {
        inet::B totalLength(stream.getRemainingLength());
        if (totalLength.get() < 8)
            throw omnetpp::cRuntimeError("Truncated ECHOSAR wire header");
        if (stream.readByte() != 'E' || stream.readByte() != 'C' ||
                stream.readByte() != 'H' || stream.readByte() != 'O')
            throw omnetpp::cRuntimeError("Invalid ECHOSAR wire magic");
        uint8_t version = stream.readUint8();
        uint8_t type = stream.readUint8();
        uint16_t bodyLength = stream.readUint16Be();
        if (version != WIRE_VERSION || type != SarCodec<T>::TYPE ||
                bodyLength > totalLength.get() - 8)
            throw omnetpp::cRuntimeError("Invalid ECHOSAR wire header");

        auto chunk = inet::makeShared<T>();
        SarCodec<T>::decode(stream, chunk);
        while (stream.getRemainingLength() >= inet::B(1))
            stream.readByte();
        chunk->setChunkLength(totalLength);
        totalDeserializedLength += totalLength;
        return chunk;
    }
};

using PositionUpdateChunkSerializer = SarChunkSerializer<PositionUpdateChunk>;
using VictimAckChunkSerializer = SarChunkSerializer<VictimAckChunk>;
using VictimAlertChunkSerializer = SarChunkSerializer<VictimAlertChunk>;

} // namespace echosar

namespace inet {

Register_Serializer(echosar::PositionUpdateChunk, echosar::PositionUpdateChunkSerializer);
Register_Serializer(echosar::VictimAckChunk, echosar::VictimAckChunkSerializer);
Register_Serializer(echosar::VictimAlertChunk, echosar::VictimAlertChunkSerializer);

} // namespace inet

#ifndef DMC_PROTOCOL_H
#define DMC_PROTOCOL_H

#include <string>
#include <vector>

#include <AnalyzerTypes.h>

namespace DMCProtocol
{

static const U16 MSG_FLAG_ACK = 0x8000;

enum ResponseCode
{
    ACK_OK = 0x0010,
    ACK_ERR_CHECKSUM = 0x0011,
    ACK_ERR_MOVING = 0x0012,
    ACK_ERR_UNSUPPORTED = 0x0013,
    ACK_ERR_RANGE = 0x0014,
    ACK_ERR_GENERAL = 0x0015,
    ACK_ERR_NOT_IN_POSITION = 0x0016,
    ACK_ERR_PREROLL = 0x0017,
    ACK_ERR_POSTROLL = 0x0018
};

struct ByteSample
{
    U8 value;
    U64 start;
    U64 end;
    bool framing_error;
};

struct Field
{
    std::string key;
    std::string value;
};

struct Packet
{
    std::vector<U8> raw;
    U64 start_sample;
    U64 end_sample;
    U32 id;
    U16 type;
    U16 length;
    U16 checksum;
    bool checksum_valid;
    bool known_type;
    bool truncated;
    bool framing_error;
    std::string type_name;
    std::string status;
    std::string direction;
    std::vector<Field> fields;
};

U16 ComputeChecksum(const U8* data, size_t length);
std::string MessageTypeName(U16 type);
std::string ResponseCodeName(U16 code);

// Incremental packet assembler. It retains only the current candidate packet.
class StreamParser
{
public:
    StreamParser();

    void Reset();
    void Push(const ByteSample& byte, std::vector<Packet>& completed);
    void Finish(std::vector<Packet>& completed);

private:
    void Emit(std::vector<Packet>& completed, bool truncated);
    std::vector<ByteSample> mCandidate;
    size_t mExpectedLength;
};

Packet ParsePacket(const std::vector<ByteSample>& bytes, bool truncated = false);

} // namespace DMCProtocol

#endif

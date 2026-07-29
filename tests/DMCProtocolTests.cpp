#include "DMCProtocol.h"

#include <cassert>
#include <vector>

using namespace DMCProtocol;

static std::vector<U8> MakePacket(U32 id, U16 type, const std::vector<U8>& data, bool corrupt = false)
{
    std::vector<U8> p{'D','F', static_cast<U8>(id), static_cast<U8>(id >> 8), static_cast<U8>(id >> 16), static_cast<U8>(id >> 24), static_cast<U8>(type), static_cast<U8>(type >> 8), static_cast<U8>(data.size()), static_cast<U8>(data.size() >> 8)};
    p.insert(p.end(), data.begin(), data.end());
    U16 sum1 = 0, sum2 = 0;
    for (size_t i = 0; i < p.size(); ++i) { sum1 = static_cast<U16>((sum1 + p[i]) % 0xff); sum2 = static_cast<U16>((sum2 + sum1) % 0xff); }
    U8 c0 = static_cast<U8>(0xff - ((sum1 + sum2) % 0xff));
    U8 c1 = static_cast<U8>(0xff - ((sum1 + c0) % 0xff));
    p.push_back(corrupt ? static_cast<U8>(c0 ^ 1) : c0); p.push_back(c1);
    return p;
}

static std::vector<ByteSample> Samples(const std::vector<U8>& raw)
{
    std::vector<ByteSample> result;
    for (size_t i = 0; i < raw.size(); ++i) result.push_back(ByteSample{raw[i], i * 10, i * 10 + 9, false});
    return result;
}

int main()
{
    std::vector<U8> data{0, 1, 2, 3};
    std::vector<U8> raw = MakePacket(0x12345678, 0x0020, data);
    assert(ComputeChecksum(&raw[0], raw.size()) == 0);
    Packet packet = ParsePacket(Samples(raw));
    assert(packet.id == 0x12345678 && packet.type == 0x0020 && packet.length == 4);
    assert(packet.checksum_valid && packet.status == "valid");
    assert(packet.type_name == "MSG_DMX");
    assert(packet.fields.size() >= 3);

    std::vector<Packet> completed;
    StreamParser parser;
    std::vector<U8> bad = MakePacket(2, 0x0020, data, true);
    std::vector<U8> noise{'x', 'D', 'x'};
    for (size_t i = 0; i < noise.size(); ++i) parser.Push(ByteSample{noise[i], i, i + 1, false}, completed);
    for (size_t i = 0; i < bad.size(); ++i) parser.Push(ByteSample{bad[i], 100 + i, 101 + i, false}, completed);
    for (size_t i = 0; i < raw.size(); ++i) parser.Push(ByteSample{raw[i], 200 + i, 201 + i, false}, completed);
    assert(completed.size() == 2);
    assert(!completed[0].checksum_valid && completed[0].status == "checksum error");
    assert(completed[1].checksum_valid);

    std::vector<ByteSample> framing = Samples(raw);
    framing[3].framing_error = true;
    Packet framing_packet = ParsePacket(framing);
    assert(framing_packet.framing_error && framing_packet.status == "framing error");

    parser.Reset();
    for (size_t i = 0; i + 1 < raw.size(); ++i) parser.Push(ByteSample{raw[i], i, i + 1, false}, completed);
    completed.clear();
    parser.Finish(completed);
    assert(completed.size() == 1 && completed[0].truncated);
    return 0;
}

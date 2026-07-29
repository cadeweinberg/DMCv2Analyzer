#include "DMCProtocol.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace DMCProtocol
{

namespace
{
U16 Word(const std::vector<U8>& b, size_t p)
{ return static_cast<U16>(b[p] | (static_cast<U16>(b[p + 1]) << 8)); }

U32 Dword(const std::vector<U8>& b, size_t p)
{ return static_cast<U32>(b[p] | (static_cast<U32>(b[p + 1]) << 8) |
                          (static_cast<U32>(b[p + 2]) << 16) |
                          (static_cast<U32>(b[p + 3]) << 24)); }

std::string Hex(U64 value, unsigned width = 0)
{
    std::ostringstream s;
    s << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(width) << value;
    return s.str();
}

void Add(Packet& p, const std::string& key, const std::string& value)
{ p.fields.push_back(Field{key, value}); }

void AddWord(Packet& p, const std::vector<U8>& b, size_t& o, const std::string& key)
{ if (o + 2 <= b.size()) { Add(p, key, Hex(Word(b, o), 4)); o += 2; } }

void AddDword(Packet& p, const std::vector<U8>& b, size_t& o, const std::string& key)
{ if (o + 4 <= b.size()) { Add(p, key, Hex(Dword(b, o), 8)); o += 4; } }

void AddSignedDword(Packet& p, const std::vector<U8>& b, size_t& o, const std::string& key)
{ if (o + 4 <= b.size()) { Add(p, key, std::to_string(static_cast<S32>(Dword(b, o)))); o += 4; } }

void AddDecimalDword(Packet& p, const std::vector<U8>& b, size_t& o, const std::string& key)
{ if (o + 4 <= b.size()) { Add(p, key, std::to_string(Dword(b, o))); o += 4; } }

void AddScaledDword(Packet& p, const std::vector<U8>& b, size_t& o, const std::string& key, double scale)
{
    if (o + 4 <= b.size()) {
        std::ostringstream value;
        value << std::fixed << std::setprecision(5) << (static_cast<double>(Dword(b, o)) / scale);
        Add(p, key, value.str());
        o += 4;
    }
}

void AddByte(Packet& p, const std::vector<U8>& b, size_t& o, const std::string& key)
{ if (o < b.size()) { Add(p, key, Hex(b[o], 2)); ++o; } }

void AddRemaining(Packet& p, const std::vector<U8>& b, size_t o)
{
    if (o < b.size()) {
        std::ostringstream s;
        for (; o < b.size(); ++o) s << (o == 0 ? "" : " ") << Hex(b[o], 2);
        Add(p, "RawData", s.str());
    }
}

void DecodeFields(Packet& p)
{
    const std::vector<U8> data(p.raw.begin() + 10, p.raw.begin() + 10 + p.length);
    size_t o = 0;
    if ((p.type & MSG_FLAG_ACK) != 0) {
        p.direction = "response";
        if (data.size() >= 2) Add(p, "Response", ResponseCodeName(Word(data, 0)) + " " + Hex(Word(data, 0), 4));
        AddRemaining(p, data, std::min<size_t>(2, data.size()));
        return;
    }
    p.direction = "request/unsolicited";
    switch (p.type) {
    case 0x0001: // HI response is the principal unsolicited non-ACK packet.
        if (data.size() >= 32) {
            std::string name(reinterpret_cast<const char*>(&data[0]), 32);
            name.erase(std::find(name.begin(), name.end(), '\0'), name.end());
            Add(p, "Name", name);
            AddByte(p, data, o = 32, "FirmwareMajor"); AddByte(p, data, o, "FirmwareMinor"); AddByte(p, data, o, "FirmwareRevision");
            AddByte(p, data, o, "MotorCount"); AddWord(p, data, o, "DMXCount"); AddByte(p, data, o, "GioOutputCount");
            AddByte(p, data, o, "GioInputCount"); AddByte(p, data, o, "HardwareLimitCount"); AddDword(p, data, o, "UploadFrameCount");
            AddDword(p, data, o, "Capabilities"); AddWord(p, data, o, "ProtocolVersion");
        }
        break;
    case 0x0020:
        AddByte(p, data, o, "Ramp"); AddWord(p, data, o, "StartChannel");
        for (size_t i = 0; o < data.size(); ++i) AddByte(p, data, o, "LightValue" + std::to_string(i));
        break;
    case 0x0021: case 0x0022: AddDword(p, data, o, "Triggers"); break;
    case 0x0023: AddDword(p, data, o, "Triggers"); break;
    case 0x0030: AddDword(p, data, o, "MotorStatus"); AddByte(p, data, o, "DMXStatus"); break;
    case 0x0031: AddByte(p, data, o, "Motor"); AddSignedDword(p, data, o, "Position"); break;
    case 0x0032: case 0x0035: case 0x0037: case 0x0038: case 0x0039:
        AddByte(p, data, o, "Motor");
        if (p.type == 0x0035) AddSignedDword(p, data, o, "Position");
        if (p.type == 0x0037) AddByte(p, data, o, "Flags");
        if (p.type == 0x0038) { AddDword(p, data, o, "MaxVelocity"); AddDword(p, data, o, "MaxAcceleration"); }
        if (p.type == 0x0039) { AddByte(p, data, o, "LowerEnable"); AddDword(p, data, o, "LowerLimit"); AddByte(p, data, o, "UpperEnable"); AddDword(p, data, o, "UpperLimit"); AddByte(p, data, o, "HardwareLimitSet"); }
        break;
    case 0x0034: { AddDword(p, data, o, "MoveTime"); size_t index = 1; while (o + 4 <= data.size()) AddSignedDword(p, data, o, "MotorPosition" + std::to_string(index++)); break; }
    case 0x0036: AddByte(p, data, o, "Motor"); AddWord(p, data, o, "Speed"); AddSignedDword(p, data, o, "Destination"); break;
    case 0x003A: AddByte(p, data, o, "Reason"); AddByte(p, data, o, "Motor"); break;
    case 0x0100: AddDword(p, data, o, "StartFrame"); AddDword(p, data, o, "EndFrame"); break;
    case 0x0101: { AddByte(p, data, o, "Motor"); AddDword(p, data, o, "StartIndex"); size_t index = 1; while (o + 4 <= data.size()) AddDword(p, data, o, "Position" + std::to_string(index++)); break; }
    case 0x0102: { AddWord(p, data, o, "Channel"); AddDword(p, data, o, "StartIndex"); size_t index = 1; while (o < data.size()) AddByte(p, data, o, "Level" + std::to_string(index++)); break; }
    case 0x0104: { AddDword(p, data, o, "Mask"); size_t index = 1; while (o + 8 <= data.size()) { AddDword(p, data, o, "Frame" + std::to_string(index)); AddDword(p, data, o, "Values" + std::to_string(index++)); } break; }
    case 0x0110: case 0x0120: AddDword(p, data, o, "Frame"); break;
    case 0x0111: AddDword(p, data, o, "FPS"); AddDword(p, data, o, "StartFrame"); AddDword(p, data, o, "EndFrame"); AddDword(p, data, o, "PreRollTime"); AddDword(p, data, o, "PostRollTime"); AddByte(p, data, o, "SyncDMX"); AddDword(p, data, o, "BloopLocation"); AddWord(p, data, o, "BloopDMXChannel"); AddWord(p, data, o, "BloopTime"); AddWord(p, data, o, "Flags"); break;
    case 0x0112: { AddDword(p, data, o, "Frame"); AddByte(p, data, o, "Direction"); AddDword(p, data, o, "ExposureTime"); AddWord(p, data, o, "BlurPercent"); size_t index = 1; while (o + 9 <= data.size()) { AddByte(p, data, o, "Motor" + std::to_string(index)); AddSignedDword(p, data, o, "PositionA" + std::to_string(index)); AddSignedDword(p, data, o, "PositionB" + std::to_string(index++)); } break; }
    case 0x0115: { AddDword(p, data, o, "Frame"); AddDword(p, data, o, "ExposureTime"); AddWord(p, data, o, "OpenAngle"); AddWord(p, data, o, "CloseAngle"); size_t index = 1; while (o + 9 <= data.size()) { AddByte(p, data, o, "Motor" + std::to_string(index)); AddSignedDword(p, data, o, "PositionA" + std::to_string(index)); AddSignedDword(p, data, o, "PositionB" + std::to_string(index++)); } break; }
    case 0x0200: {
        AddByte(p, data, o, "VirtualType");
        const char* axes[] = { "Boom", "Swing", "Track", "Pan", "Tilt", "Roll" };
        U8 virtual_type = data.empty() ? 0 : data[0];
        if (virtual_type == 1) {
            for (size_t axis = 0; axis < 6; ++axis) {
                AddDecimalDword(p, data, o, std::string(axes[axis]) + "Motor");
                AddDecimalDword(p, data, o, std::string(axes[axis]) + "SPU");
                AddScaledDword(p, data, o, std::string(axes[axis]) + "Position", 100000.0);
            }
            AddScaledDword(p, data, o, "BoomLength", 1000.0);
            AddScaledDword(p, data, o, "BoomExtension", 1000.0);
            AddScaledDword(p, data, o, "NodalOffsetX", 1000.0);
            AddScaledDword(p, data, o, "NodalOffsetY", 1000.0);
            AddScaledDword(p, data, o, "NodalOffsetZ", 1000.0);
            // The compensation table is optional. If fewer than 121 DWORDs
            // remain, the optional value is SafeDistance instead.
            if (data.size() - o >= 121 * 4) {
                for (size_t index = 0; index < 121; ++index)
                    AddScaledDword(p, data, o, "BoomCompensation" + std::to_string(static_cast<int>(index) - 60), 1.0);
            }
            AddScaledDword(p, data, o, "SafeDistance", 1000.0);
        } else if (virtual_type == 2) {
            AddDecimalDword(p, data, o, "SwingMotor");
            AddDecimalDword(p, data, o, "SwingSPU");
            AddDecimalDword(p, data, o, "PanMotor");
            AddDecimalDword(p, data, o, "PanSPU");
        }
        break;
    }
    case 0x0201: AddByte(p, data, o, "VirtualMotor"); AddDword(p, data, o, "Position"); break;
    case 0x0202: AddByte(p, data, o, "VirtualMotor"); break;
    case 0x0203: AddByte(p, data, o, "Motor"); AddWord(p, data, o, "Speed"); AddDword(p, data, o, "Destination"); break;
    case 0x0205: { size_t index = 1; while (o + 4 <= data.size()) AddDword(p, data, o, "VirtualPosition" + std::to_string(index++)); if (o < data.size()) AddByte(p, data, o, "AimPointEnabled"); break; }
    case 0x0206: AddByte(p, data, o, "Axis"); AddWord(p, data, o, "Speed"); break;
    case 0x0207: AddByte(p, data, o, "Enable"); AddDword(p, data, o, "AimX"); AddDword(p, data, o, "AimY"); AddDword(p, data, o, "AimZ"); break;
    default: break;
    }
    AddRemaining(p, data, o);
}
}

U16 ComputeChecksum(const U8* data, size_t length)
{
    U16 sum1 = 0, sum2 = 0;
    while (length) {
        size_t block = std::min<size_t>(20, length);
        while (block--) { sum1 = static_cast<U16>(sum1 + *data++); sum2 = static_cast<U16>(sum2 + sum1); }
        sum1 = static_cast<U16>(sum1 % 0xff); sum2 = static_cast<U16>(sum2 % 0xff);
        length -= std::min<size_t>(20, length);
    }
    return static_cast<U16>((sum2 << 8) | sum1);
}

std::string MessageTypeName(U16 type)
{
    switch (type & ~MSG_FLAG_ACK) {
    case 0x0001: return "MSG_HI"; case 0x0020: return "MSG_DMX"; case 0x0021: return "MSG_GIO_OUT"; case 0x0022: return "MSG_GIO_IN"; case 0x0023: return "MSG_GIO_CAM";
    case 0x0030: return "MSG_MOTOR_STATUS"; case 0x0031: return "MSG_MOTOR_MOVE"; case 0x0032: return "MSG_MOTOR_STOP"; case 0x0033: return "MSG_MOTOR_STOP_ALL"; case 0x0034: return "MSG_MOTOR_GET_POSITION"; case 0x0035: return "MSG_MOTOR_RESET_POSITION"; case 0x0036: return "MSG_MOTOR_JOG"; case 0x0037: return "MSG_MOTOR_CONFIGURE"; case 0x0038: return "MSG_MOTOR_SET_SPEED"; case 0x0039: return "MSG_MOTOR_SET_LIMITS"; case 0x003A: return "MSG_MOTOR_HARD_STOP";
    case 0x0100: return "MSG_RT_UPLOAD_MOVE_BEGIN"; case 0x0101: return "MSG_RT_UPLOAD_MOVE_AXIS"; case 0x0102: return "MSG_RT_UPLOAD_MOVE_DMX"; case 0x0103: return "MSG_RT_UPLOAD_MOVE_END"; case 0x0104: return "MSG_RT_UPLOAD_MOVE_TRIGGERS"; case 0x0110: return "MSG_RT_POSITION_FRAME"; case 0x0111: return "MSG_RT_RUN_MOVE"; case 0x0112: return "MSG_RT_SHOOT_FRAME"; case 0x0113: return "MSG_RT_GO"; case 0x0114: return "MSG_RT_END"; case 0x0115: return "MSG_RT_SHOOT_FRAME2"; case 0x0116: return "MSG_RT_STOP_LOOP"; case 0x0120: return "MSG_RT_JOG_ALL";
    case 0x0200: return "MSG_VIRT_CONFIG"; case 0x0201: return "MSG_VIRT_MOVE"; case 0x0202: return "MSG_VIRT_STOP"; case 0x0203: return "MSG_VIRT_JOG"; case 0x0205: return "MSG_VIRT_GET_POSITION"; case 0x0206: return "MSG_VIRT_JOG_ON_LINE"; case 0x0207: return "MSG_VIRT_AIM_POINT"; default: return "UNKNOWN";
    }
}

std::string ResponseCodeName(U16 code)
{
    switch (code) { case ACK_OK: return "OK"; case ACK_ERR_CHECKSUM: return "ERR_CHECKSUM"; case ACK_ERR_MOVING: return "ERR_MOVING"; case ACK_ERR_UNSUPPORTED: return "ERR_UNSUPPORTED"; case ACK_ERR_RANGE: return "ERR_RANGE"; case ACK_ERR_GENERAL: return "ERR_GENERAL"; case ACK_ERR_NOT_IN_POSITION: return "ERR_NOT_IN_POSITION"; case ACK_ERR_PREROLL: return "ERR_PREROLL"; case ACK_ERR_POSTROLL: return "ERR_POSTROLL"; default: return "UNKNOWN_RESPONSE"; }
}

Packet ParsePacket(const std::vector<ByteSample>& bytes, bool truncated)
{
    Packet p{}; p.truncated = truncated; p.framing_error = false; p.start_sample = bytes.empty() ? 0 : bytes.front().start; p.end_sample = bytes.empty() ? 0 : bytes.back().end;
    for (size_t i = 0; i < bytes.size(); ++i) p.raw.push_back(bytes[i].value);
    for (size_t i = 0; i < bytes.size(); ++i) p.framing_error = p.framing_error || bytes[i].framing_error;
    p.status = truncated ? "truncated" : "invalid";
    if (p.raw.size() >= 12 && p.raw[0] == 'D' && p.raw[1] == 'F') {
        p.id = Dword(p.raw, 2); p.type = Word(p.raw, 6); p.length = Word(p.raw, 8); p.type_name = MessageTypeName(p.type); p.known_type = p.type_name != "UNKNOWN";
        if (p.raw.size() >= static_cast<size_t>(12 + p.length)) {
            p.checksum = Word(p.raw, 10 + p.length); p.checksum_valid = ComputeChecksum(&p.raw[0], 12 + p.length) == 0; p.status = p.framing_error ? "framing error" : (p.checksum_valid ? "valid" : "checksum error"); DecodeFields(p);
        }
    }
    return p;
}

StreamParser::StreamParser() : mExpectedLength(0) {}
void StreamParser::Reset() { mCandidate.clear(); mExpectedLength = 0; }
void StreamParser::Emit(std::vector<Packet>& out, bool truncated) { if (!mCandidate.empty()) out.push_back(ParsePacket(mCandidate, truncated)); Reset(); }
void StreamParser::Push(const ByteSample& byte, std::vector<Packet>& out)
{
    if (mCandidate.empty()) { if (byte.value == 'D') mCandidate.push_back(byte); return; }
    if (mCandidate.size() == 1 && byte.value != 'F') { if (byte.value == 'D') mCandidate[0] = byte; else Reset(); return; }
    mCandidate.push_back(byte);
    if (mCandidate.size() == 10) mExpectedLength = 12 + Word(std::vector<U8>{mCandidate[8].value,mCandidate[9].value}, 0);
    if (mExpectedLength && mCandidate.size() >= mExpectedLength) Emit(out, false);
}
void StreamParser::Finish(std::vector<Packet>& out) { if (!mCandidate.empty()) Emit(out, true); }

} // namespace DMCProtocol

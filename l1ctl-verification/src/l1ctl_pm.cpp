#include "l1ctl_pm.hpp"

namespace l1ctl {

ParseResult parsePmConf(const std::uint8_t* bytes, std::size_t length) {
    ParseResult result;
    if (length < kHeaderSize || bytes == nullptr) {
        result.error = ParseError::TruncatedHeader;
        return result;
    }
    if (bytes[0] != kPmConf) {
        result.error = ParseError::WrongMessageType;
        return result;
    }
    result.flags = bytes[1];
    const std::size_t payloadLength = length - kHeaderSize;
    if (payloadLength == 0) {
        result.error = ParseError::MissingMeasurement;
        return result;
    }
    if (payloadLength % kPmRecordSize != 0) {
        result.error = ParseError::TruncatedMeasurement;
        return result;
    }

    result.measurements.reserve(payloadLength / kPmRecordSize);
    for (std::size_t i = kHeaderSize; i < length; i += kPmRecordSize) {
        const auto arfcn = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(bytes[i]) << 8) | bytes[i + 1]);
        result.measurements.push_back({arfcn, bytes[i + 2], bytes[i + 3]});
    }
    return result;
}

const char* toString(ParseError error) noexcept {
    switch (error) {
        case ParseError::None: return "none";
        case ParseError::TruncatedHeader: return "truncated header";
        case ParseError::WrongMessageType: return "wrong message type";
        case ParseError::MissingMeasurement: return "missing measurement";
        case ParseError::TruncatedMeasurement: return "truncated measurement";
    }
    return "unknown";
}

} // namespace l1ctl

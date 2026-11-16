#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace l1ctl {

/** @brief L1CTL message type for a power-measurement confirmation. */
constexpr std::uint8_t kPmConf = 0x09;

/** @brief Size of `l1ctl_hdr` in the supplied OsmocomBB checkout, in bytes. */
constexpr std::size_t kHeaderSize = 4;

/** @brief Size of a packed `l1ctl_pm_conf` record, in bytes. */
constexpr std::size_t kPmRecordSize = 4;

/** @brief A decoded power-measurement record.
 *
 * `pm0` and `pm1` retain the raw bytes from the firmware. The supplied
 * host-side code forwards `pm[0]` as the received level, but this parser
 * deliberately does not interpret or calibrate either measurement field.
 */
struct PowerMeasurement {
    std::uint16_t band_arfcn;
    std::uint8_t pm0;
    std::uint8_t pm1;
};

/** @brief Reasons a PM_CONF message can be rejected. */
enum class ParseError {
    None,
    TruncatedHeader,
    WrongMessageType,
    MissingMeasurement,
    TruncatedMeasurement
};

/** @brief The result of decoding one L1CTL PM_CONF message. */
struct ParseResult {
    ParseError error{ParseError::None};
    std::uint8_t flags{0};
    std::vector<PowerMeasurement> measurements;

    /** @return True if the complete message was decoded successfully. */
    [[nodiscard]] bool ok() const noexcept { return error == ParseError::None; }
};

/** @brief Decode a complete L1CTL_PM_CONF message into typed records.
 *
 * @param bytes Pointer to the first byte of the 4-byte L1CTL header.
 * @param length Total number of bytes, including the L1CTL header.
 * @return A result with either all measurements or a descriptive error.
 *
 * @pre `bytes` points to at least `length` readable bytes when `length > 0`.
 * @note The 16-bit `band_arfcn` field uses network byte order, as shown by
 *       `ntohs(pmr->band_arfcn)` in the provided OsmocomBB host code.
 * @note This function operates on a complete application-level L1CTL message,
 *       not on the socket's separate transport-length prefix.
 * @note No radio messages are transmitted; this is an offline parser.
 */
[[nodiscard]] ParseResult parsePmConf(const std::uint8_t* bytes, std::size_t length);

/** @brief Human-readable name for a parser result code. */
[[nodiscard]] const char* toString(ParseError error) noexcept;

} // namespace l1ctl

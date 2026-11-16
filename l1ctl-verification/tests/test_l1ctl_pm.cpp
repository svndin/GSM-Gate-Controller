#include "l1ctl_pm.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

/** @brief Minimal standalone test runner, without external dependencies. */
static void expect(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main() {
    using namespace l1ctl;

    // L1CTL header: type=0x09, flags=0x01, two reserved bytes.
    // PM record: band_arfcn=0x0123 (network order), pm[0]=55, pm[1]=77.
    const std::vector<std::uint8_t> valid{
        0x09, 0x01, 0x00, 0x00, 0x01, 0x23, 55, 77
    };
    auto parsed = parsePmConf(valid.data(), valid.size());
    expect(parsed.ok(), "valid PM_CONF accepted");
    expect(parsed.flags == 0x01, "flags preserved");
    expect(parsed.measurements.size() == 1, "one measurement decoded");
    expect(parsed.measurements[0].band_arfcn == 0x0123,
           "big-endian ARFCN decoded correctly");
    expect(parsed.measurements[0].pm0 == 55 &&
           parsed.measurements[0].pm1 == 77, "raw measurements preserved");

    const std::vector<std::uint8_t> twoRecords{
        0x09, 0x00, 0x11, 0x22,
        0x00, 0x40, 47, 99,
        0x00, 0x48, 52, 100
    };
    parsed = parsePmConf(twoRecords.data(), twoRecords.size());
    expect(parsed.ok() && parsed.measurements.size() == 2,
           "two packed measurements accepted");
    expect(parsed.measurements[1].band_arfcn == 72,
           "second ARFCN decoded correctly");

    expect(parsePmConf(nullptr, 0).error == ParseError::TruncatedHeader,
           "empty input rejected");
    expect(parsePmConf(valid.data(), 3).error == ParseError::TruncatedHeader,
           "incomplete header rejected");
    auto wrongType = valid;
    wrongType[0] = 0x08; // L1CTL_PM_REQ, not PM_CONF.
    expect(parsePmConf(wrongType.data(), wrongType.size()).error ==
               ParseError::WrongMessageType,
           "unexpected message type rejected");
    expect(parsePmConf(valid.data(), 4).error == ParseError::MissingMeasurement,
           "header-only message rejected");
    expect(parsePmConf(valid.data(), 7).error == ParseError::TruncatedMeasurement,
           "partial record rejected");
    auto trailing = valid;
    trailing.push_back(0xff);
    expect(parsePmConf(trailing.data(), trailing.size()).error ==
               ParseError::TruncatedMeasurement,
           "trailing byte rejected");

    std::cout << "PASS: 10 L1CTL PM_CONF parser checks\n";
    return EXIT_SUCCESS;
}

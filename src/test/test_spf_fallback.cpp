/*
 * This file is part of jASTERIX.
 *
 * jASTERIX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * jASTERIX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with jASTERIX.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"
#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"

using namespace std;
using namespace nlohmann;

// REF/SPF length-mismatch fallback: when the content of a REF/SPF does not match
// the selected definition, the field must be kept as a raw hex string with a
// ref_error/spf_error flag in the record, and the rest of the data block must
// decode normally (no decode error).
//
// SPF cases use a live CAT062 ed 1.21 record (track number 6428, from
// cat062ed1.21_spf.bin) with the ARTAS SPF (REP + 4-byte TRIs). Foreign variants
// carry SPF content whose parsed size does not match the announced length -
// as produced by equipment writing other content into the SPF (the 4a1b8000
// case is from a customer CAT021 recording that previously failed the whole
// data block).
//
// The REF case uses a live CAT021 ed 2.4 record with the RE FSPEC bit set and
// the same foreign content against the CAT021 REF 1.5 definition.

namespace
{

// CAT062 record (71 bytes in original) without the trailing 6-byte SPF field
const std::string cat062_base_hex =
    "bf8f81070232f0e14652a60088c73b002ff22f067e24021bcf00c5fef30201191cb9"
    "23013875200affffffff0cbf80010900b50098014bff095c480a0dff80320b";

// proper ARTAS SPF: length 6 (incl. len byte), REP 1, TRI f052a356
const std::string spf_proper_hex = "0601f052a356";

// foreign SPF, mismatch case: announces 4 data bytes, but REP 1 makes the
// definition read 5 (1 REP byte + one 4-byte TRI)
const std::string spf_mismatch_hex = "0501aabbcc";

// foreign SPF, exception case: announces 4 data bytes, REP 0x4a (74) makes the
// definition read past the end of the buffer
const std::string spf_foreign_hex = "054a1b8000";

// CAT021 record without the trailing REF field; FSPEC octet 7 (0x04) selects RE
const std::string cat021_base_ref_hex =
    "d51d3343810704681d03500117bf7ceaac9dc1f9007e8e9b0dec7f00171612"
    "0000007e8e9b0494853cec31005181180000000000";

// CAT021 record without the trailing SPF field; FSPEC octet 7 (0x02) selects SP
const std::string cat021_base_spf_hex =
    "d51d3343810702681d03500117bf7ceaac9dc1f9007e8e9b0dec7f00171612"
    "0000007e8e9b0494853cec31005181180000000000";

std::vector<char> hex2bin(const std::string& hex)
{
    assert(hex.size() % 2 == 0);

    std::vector<char> data;
    data.reserve(hex.size() / 2);

    for (size_t cnt = 0; cnt < hex.size(); cnt += 2)
        data.push_back(static_cast<char>(stoull(hex.substr(cnt, 2), nullptr, 16)));

    return data;
}

// assemble a data block from record hex strings
std::vector<char> makeDataBlock(unsigned int cat, const std::vector<std::string>& record_hexs)
{
    std::string content_hex;
    for (const auto& rec_hex : record_hexs)
        content_hex += rec_hex;

    std::vector<char> content = hex2bin(content_hex);
    size_t block_len = 3 + content.size();

    std::vector<char> block;
    block.push_back(static_cast<char>(cat));
    block.push_back(static_cast<char>((block_len >> 8) & 0xff));
    block.push_back(static_cast<char>(block_len & 0xff));
    block.insert(block.end(), content.begin(), content.end());

    return block;
}

void configureCat062(jASTERIX::jASTERIX& jasterix)
{
    REQUIRE(jasterix.hasCategory(62));
    std::shared_ptr<jASTERIX::Category> cat062 = jasterix.category(62);
    REQUIRE(cat062->hasEdition("1.21"));
    cat062->setCurrentEdition("1.21");
    REQUIRE(cat062->hasSPFEdition("ARTAS"));
    cat062->setCurrentSPFEdition("ARTAS");
}

void configureCat021(jASTERIX::jASTERIX& jasterix)
{
    REQUIRE(jasterix.hasCategory(21));
    std::shared_ptr<jASTERIX::Category> cat021 = jasterix.category(21);
    REQUIRE(cat021->hasEdition("2.4"));
    cat021->setCurrentEdition("2.4");
    REQUIRE(cat021->hasREFEdition("1.5"));
    cat021->setCurrentREFEdition("1.5");
}

void checkProperSPF(const json& record)
{
    REQUIRE(record.at("040").at("Track Number") == 6428);

    REQUIRE(record.contains("SPF"));
    const json& spf = record.at("SPF");

    REQUIRE(spf.is_object());
    REQUIRE(spf.at("REP") == 1);
    REQUIRE(spf.at("Target Report Identifiers").is_array());
    REQUIRE(spf.at("Target Report Identifiers").size() == 1);
    REQUIRE(spf.at("Target Report Identifiers").at(0).at("TRI") == "f052a356");

    REQUIRE(!record.contains("spf_error"));
}

void checkFallbackSPF(const json& record, const std::string& hex)
{
    REQUIRE(record.contains("SPF"));
    REQUIRE(record.at("SPF").is_string());
    REQUIRE(record.at("SPF") == hex);

    REQUIRE(record.contains("spf_error"));
    REQUIRE(record.at("spf_error") == true);
}

}  // anonymous namespace

TEST_CASE("jASTERIX SPF fallback", "[jASTERIX SPF fallback]")
{
    loginf << "spf fallback test: start" << logendl;

    SECTION("structured, foreign SPF between proper SPFs")
    {
        // foreign SPF mid-block: the definition reads past the announced field end
        // into the next record, the length mismatch triggers the fallback
        std::vector<char> block =
            makeDataBlock(62, {cat062_base_hex + spf_proper_hex,
                               cat062_base_hex + spf_mismatch_hex,
                               cat062_base_hex + spf_proper_hex});

        jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
        configureCat062(jasterix);

        jasterix.decodeData(block.data(), block.size(),
                            [](unique_ptr<json> json_data, size_t, size_t num_frames,
                               size_t num_records, size_t num_errors)
                            {
                                REQUIRE(num_frames == 0);
                                REQUIRE(num_records == 3);
                                REQUIRE(num_errors == 0);

                                const json& records =
                                    json_data->at("data_blocks").at(0).at("content").at("records");
                                REQUIRE(records.size() == 3);

                                checkProperSPF(records.at(0));
                                checkFallbackSPF(records.at(1), "01aabbcc");
                                checkProperSPF(records.at(2));
                            },
                            false);

        REQUIRE(jasterix.numSPFErrors() == 1);
        REQUIRE(jasterix.numREFErrors() == 0);
    }

    SECTION("structured, foreign SPF at end of buffer")
    {
        // foreign SPF as very last field: REP 0x4a makes the definition read far
        // beyond the buffer, the resulting exception triggers the fallback
        std::vector<char> block = makeDataBlock(62, {cat062_base_hex + spf_foreign_hex});

        jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
        configureCat062(jasterix);

        jasterix.decodeData(block.data(), block.size(),
                            [](unique_ptr<json> json_data, size_t, size_t,
                               size_t num_records, size_t num_errors)
                            {
                                REQUIRE(num_records == 1);
                                REQUIRE(num_errors == 0);

                                const json& records =
                                    json_data->at("data_blocks").at(0).at("content").at("records");

                                checkFallbackSPF(records.at(0), "4a1b8000");
                            },
                            false);

        REQUIRE(jasterix.numSPFErrors() == 1);
    }

    SECTION("structured, foreign REF")
    {
        // foreign content in the CAT021 REF: does not match the REF 1.5
        // definition, must fall back to hex with ref_error
        std::vector<char> block =
            makeDataBlock(21, {cat021_base_ref_hex + spf_foreign_hex,
                               cat021_base_spf_hex + spf_foreign_hex});

        jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
        configureCat021(jasterix);

        jasterix.decodeData(block.data(), block.size(),
                            [](unique_ptr<json> json_data, size_t, size_t,
                               size_t num_records, size_t num_errors)
                            {
                                REQUIRE(num_records == 2);
                                REQUIRE(num_errors == 0);

                                const json& records =
                                    json_data->at("data_blocks").at(0).at("content").at("records");

                                const json& record = records.at(0);
                                REQUIRE(record.at("010") ==
                                        json({{"SAC", 104}, {"SIC", 29}}));
                                REQUIRE(record.contains("REF"));
                                REQUIRE(record.at("REF").is_string());
                                REQUIRE(record.at("REF") == "4a1b8000");
                                REQUIRE(record.contains("ref_error"));
                                REQUIRE(record.at("ref_error") == true);

                                // second record carries the SP bit; without an SPF
                                // edition selected the content stays plain hex, no flag
                                const json& record2 = records.at(1);
                                REQUIRE(record2.at("SPF") == "4a1b8000");
                                REQUIRE(!record2.contains("spf_error"));
                            },
                            false);

        REQUIRE(jasterix.numREFErrors() == 1);
        REQUIRE(jasterix.numSPFErrors() == 0);
    }

    SECTION("flat, foreign SPF between proper SPFs")
    {
        // in flat mode the partially decoded SPF leaf cells must be reset to null
        std::vector<char> block =
            makeDataBlock(62, {cat062_base_hex + spf_proper_hex,
                               cat062_base_hex + spf_mismatch_hex,
                               cat062_base_hex + spf_proper_hex});

        jASTERIX::jASTERIX jasterix(definition_path, false, false, false);
        configureCat062(jasterix);

        jasterix.decodeData(block.data(), block.size(),
                            [](unique_ptr<json> json_data, size_t, size_t,
                               size_t num_records, size_t num_errors)
                            {
                                REQUIRE(num_records == 3);
                                REQUIRE(num_errors == 0);

                                REQUIRE(json_data->contains("62"));
                                const json& cat62 = json_data->at("62");

                                REQUIRE(cat62.contains("SPF.REP"));
                                const json& rep_col = cat62.at("SPF.REP");
                                REQUIRE(rep_col.size() == 3);
                                REQUIRE(rep_col.at(0) == 1);
                                REQUIRE(rep_col.at(1).is_null());
                                REQUIRE(rep_col.at(2) == 1);

                                REQUIRE(cat62.contains("SPF.Target Report Identifiers.TRI"));
                                const json& tri_col = cat62.at("SPF.Target Report Identifiers.TRI");
                                REQUIRE(tri_col.size() == 3);
                                REQUIRE(tri_col.at(0).is_array());
                                REQUIRE(tri_col.at(0).at(0) == "f052a356");
                                REQUIRE(tri_col.at(1).is_null());
                                REQUIRE(tri_col.at(2).is_array());
                                REQUIRE(tri_col.at(2).at(0) == "f052a356");
                            },
                            false,
                            true);  // do_flat = true

        REQUIRE(jasterix.numSPFErrors() == 1);
    }

    loginf << "spf fallback test: end" << logendl;
}

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
#include "files.h"
#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"

using namespace std;
using namespace nlohmann;

TEST_CASE("jASTERIX CAT001 1.1 flat SAC/SIC propagation", "[jASTERIX CAT001]")
{
    loginf << "cat001 flat sac/sic test: start" << logendl;

    jASTERIX::jASTERIX jasterix(definition_path, true, true, false);

    REQUIRE(jasterix.hasCategory(1));
    std::shared_ptr<jASTERIX::Category> cat001 = jasterix.category(1);
    REQUIRE(cat001->hasEdition("1.1"));
    cat001->setCurrentEdition("1.1");
    cat001->setCurrentMapping("");

    // Data block with 3 CAT001 records:
    //   Record 1: FSPEC=0xfe — has I001/010 (SAC=0, SIC=1), 020, 040, 070, 090, 130, 141
    //   Record 2: FSPEC=0x7e — no I001/010, has 020, 040, 070, 090, 130, 141
    //   Record 3: FSPEC=0x7e — same as record 2
    // Per CAT001 spec §5.3.2.1, SAC/SIC is only required in the first record of a data block.
    // In flat mode, the library must propagate SAC/SIC from record 1 to records 2 and 3.
    const std::string filename = "cat001ed1.1_sacsic_propagation.bin";

    REQUIRE(jASTERIX::Files::fileExists(data_path + filename));
    REQUIRE(jASTERIX::Files::fileSize(data_path + filename) == 50);

    bool callback_called = false;

    jasterix.decodeFile(data_path + filename,
        [&](std::unique_ptr<nlohmann::json> json_data, size_t total_num_bytes,
            size_t num_frames, size_t num_records, size_t num_errors)
        {
            callback_called = true;

            loginf << "cat001 flat sac/sic test: callback with " << num_records
                   << " records, " << num_errors << " errors" << logendl;

            REQUIRE(num_frames == 0);
            REQUIRE(num_records == 3);
            REQUIRE(num_errors == 0);

            // Flat output: {"1": {"010.SAC": [...], "010.SIC": [...], ...}}
            REQUIRE(json_data->contains("1"));
            const json& cat1 = json_data->at("1");

            REQUIRE(cat1.contains("010.SAC"));
            REQUIRE(cat1.contains("010.SIC"));

            const json& sac_col = cat1.at("010.SAC");
            const json& sic_col = cat1.at("010.SIC");

            REQUIRE(sac_col.is_array());
            REQUIRE(sic_col.is_array());
            REQUIRE(sac_col.size() == 3);
            REQUIRE(sic_col.size() == 3);

            // All 3 records must have SAC=0, SIC=1
            // Record 1 has it natively; records 2 and 3 get it via propagation
            for (size_t i = 0; i < 3; ++i)
            {
                REQUIRE_FALSE(sac_col[i].is_null());
                REQUIRE_FALSE(sic_col[i].is_null());
                REQUIRE(sac_col[i] == 0);
                REQUIRE(sic_col[i] == 1);
            }

            // Verify other items are also correctly decoded for all 3 records
            REQUIRE(cat1.contains("040.RHO"));
            REQUIRE(cat1.contains("040.THETA"));
            REQUIRE(cat1.contains("141.Truncated Time of Day"));

            const json& rho_col = cat1.at("040.RHO");
            const json& theta_col = cat1.at("040.THETA");
            const json& tod_col = cat1.at("141.Truncated Time of Day");

            REQUIRE(rho_col.size() == 3);
            REQUIRE(theta_col.size() == 3);
            REQUIRE(tod_col.size() == 3);

            for (size_t i = 0; i < 3; ++i)
            {
                REQUIRE_FALSE(rho_col[i].is_null());
                REQUIRE_FALSE(theta_col[i].is_null());
                REQUIRE_FALSE(tod_col[i].is_null());
                REQUIRE(approximatelyEqual(rho_col[i].get<double>(), 127.4375, 10e-4));
                REQUIRE(approximatelyEqual(theta_col[i].get<double>(), 256.61865234375, 10e-10));
                REQUIRE(approximatelyEqual(tod_col[i].get<double>(), 221.4296875, 10e-6));
            }

            loginf << "cat001 flat sac/sic test: all checks passed" << logendl;
        },
        true  // do_flat = true
    );

    REQUIRE(callback_called);

    loginf << "cat001 flat sac/sic test: end" << logendl;
}

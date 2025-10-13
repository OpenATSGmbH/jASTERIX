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

#include "hashchecker.h"

#include <sstream>

#include "jasterix.h"
#include "logger.h"
#include "traced_assert.h"

std::string check_artas_md5_hash;
std::vector<int> check_artas_md5_categories;
std::unique_ptr<HashChecker> hash_checker;

using namespace nlohmann;
// using namespace Utils;

void check_callback(std::unique_ptr<nlohmann::json> data_chunk, size_t num_frames,
                    size_t num_records, size_t num_errors)
{
    traced_assert(hash_checker);
    hash_checker->process(std::move(data_chunk));
}

HashChecker::HashChecker(bool framing_used) : framing_used_(framing_used)
{
    traced_assert(jASTERIX::add_record_data);
}

void HashChecker::process(std::unique_ptr<nlohmann::json> data)
{
    // loginf << "UGA '" << data->dump(4) << "'" << logendl;

    unsigned int category;
    //    unsigned int index;
    //    unsigned int length;

    if (!framing_used_)
    {
        traced_assert(data->find("data_blocks") != data->end());

        for (json& data_block : data->at("data_blocks"))
        {
            category = data_block.at("category");

            if (std::find(check_artas_md5_categories.begin(), check_artas_md5_categories.end(),
                          category) == check_artas_md5_categories.end())
                continue;

            if (!data_block.contains("content"))  // data blocks with errors
                continue;

            json& data_block_content = data_block.at("content");

            //            index = data_block_content.at("index");
            //            length = data_block_content.at("length");

            if (data_block_content.contains("records"))
            {
                for (json& record : data_block_content.at("records"))
                    processRecord(category, record);
            }
        }
    }
    else
    {
        traced_assert(data->find("frames") != data->end());
        traced_assert(data->at("frames").is_array());

        for (json& frame : data->at("frames"))
        {
            if (!frame.contains("content"))  // frame with errors
                continue;

            traced_assert(frame.at("content").is_object());

            if (!frame.at("content").contains("data_blocks"))  // frame with errors
                continue;

            traced_assert(frame.at("content").at("data_blocks").is_array());

            for (json& data_block : frame.at("content").at("data_blocks"))
            {
                if (!data_block.contains("category"))  // data block with errors
                    continue;

                category = data_block.at("category");

                if (std::find(check_artas_md5_categories.begin(), check_artas_md5_categories.end(),
                              category) == check_artas_md5_categories.end())
                    continue;

                if (!data_block.contains("content"))  // data block with errors
                    continue;

                json& data_block_content = data_block.at("content");

                //                index = data_block_content.at("index");
                //                length = data_block_content.at("length");

                if (data_block_content.contains("records"))
                {
                    for (json& record : data_block_content.at("records"))
                        processRecord(category, record);
                }
            }
        }
    }
}

void HashChecker::processRecord(unsigned int category, nlohmann::json& record)
{
    //    loginf << "UGA  cat " << category << " index " << index << " length " << length
    //           << " '" << record.dump(4) << "'" << logendl;

    traced_assert(record.contains("artas_md5"));
    traced_assert(record.contains("index"));
    traced_assert(record.contains("length"));
    traced_assert(record.contains("record_data"));

    hash_map_[record.at("artas_md5")].emplace_back(category, record.at("index"),
                                                   record.at("length"), record.at("record_data"));

    //    "artas_md5": "5073ed0f",
    //    "record_data": "ff02620f221f645b9c0fff08bcc175c06ddf82"
}

void HashChecker::printCollisions()
{
    unsigned int record_cnt{0};
    unsigned int same_data_collision_cnt{0};
    unsigned int different_data_collision_cnt{0};

    std::stringstream ss_same_data_indexes;
    std::map<unsigned int, unsigned int> same_data_collisions_per_cat;
    std::map<std::pair<unsigned int, unsigned int>, unsigned int> diffent_data_collisions_per_cat;

    for (auto& hash_it : hash_map_)
    {
        if (hash_it.second.size() > 1)
        {
            loginf << "found  " << hash_it.second.size() << " collisions in hash '" << hash_it.first
                   << "':" << logendl;

            // find same data collisions
            std::map<std::pair<unsigned int, std::string>, unsigned int>
                data_counts;  // (cat, record data) -> cnt
            std::pair<unsigned int, std::string> cat_and_recdata;

            std::vector<RecordInfo> cleaned_hash_map;  // removed same cat & data occurences
            ss_same_data_indexes.str("");

            for (auto& rec_it : hash_it.second)
            {
                cat_and_recdata = std::make_pair(rec_it.category_, rec_it.record_data_);
                if (data_counts.count(cat_and_recdata))  // same data found, just increase counter
                {
                    data_counts[cat_and_recdata]++;
                    ss_same_data_indexes << "," << rec_it.index_;
                }
                else  // new data found, add to cleaned map and initialize counter
                {
                    cleaned_hash_map.push_back(rec_it);
                    data_counts[cat_and_recdata] = 1;
                    ss_same_data_indexes << rec_it.index_;
                }
            }

            for (auto& cnt_it : data_counts)
            {
                if (cnt_it.second > 1)
                {
                    loginf << "\t " << cnt_it.second << " same data collisions in cat "
                           << cnt_it.first.first << " data '" << cnt_it.first.second
                           << "' at indexes " << ss_same_data_indexes.str() << logendl;
                    same_data_collision_cnt += cnt_it.second;
                    same_data_collisions_per_cat[cnt_it.first.first] += cnt_it.second;
                }
            }

            if (cleaned_hash_map.size() > 1)
            {
                loginf << "\t found different collisions:" << logendl;

                //                for (auto& rec_it : cleaned_hash_map) // hash_it.second
                //                {
                //                    loginf << "\t cat " << rec_it.category_ << " data '" <<
                //                    rec_it.record_data_
                //                           << "' index " << rec_it.index_ << logendl;
                //                    // << " length " << rec_it.length_
                //                    ++different_data_collision_cnt; // one for each
                //                }

                for (auto rec_it = cleaned_hash_map.begin(); rec_it != cleaned_hash_map.end();
                     ++rec_it)  // hash_it.second
                {
                    loginf << "\t cat " << rec_it->category_ << " data '" << rec_it->record_data_
                           << "' index " << rec_it->index_ << logendl;
                    // << " length " << rec_it.length_
                    ++different_data_collision_cnt;  // one for each

                    // iterate over rest of collisions and mark each
                    for (auto rest_rec_it = rec_it + 1; rest_rec_it != cleaned_hash_map.end();
                         ++rest_rec_it)
                    {
                        unsigned int min_cat = std::min(rec_it->category_, rest_rec_it->category_);
                        unsigned int max_cat = std::max(rec_it->category_, rest_rec_it->category_);
                        diffent_data_collisions_per_cat[std::make_pair(min_cat, max_cat)] += 1;
                    }
                }
            }
        }

        record_cnt += hash_it.second.size();
    }

    if (!record_cnt)
    {
        loginf << "no data found" << logendl;
        return;
    }

    loginf << logendl;
    loginf << "found " << same_data_collision_cnt << " same data collisions in " << record_cnt
           << " records (" << 100.0 * same_data_collision_cnt / record_cnt << "%)" << logendl;
    if (same_data_collision_cnt)
    {
        loginf << "same data collisions per category: " << logendl;
        for (auto& cat_it : same_data_collisions_per_cat)
            loginf << "\t cat " << cat_it.first << " count " << cat_it.second << " ("
                   << 100.0 * cat_it.second / record_cnt << "%)" << logendl;
        loginf << logendl;
    }

    loginf << "found " << different_data_collision_cnt << " different data collisions in "
           << record_cnt << " records (" << 100.0 * different_data_collision_cnt / record_cnt
           << "%)" << logendl;
    if (different_data_collision_cnt)
    {
        loginf << "different data collisions per category: " << logendl;
        for (auto& cat_it : diffent_data_collisions_per_cat)
            loginf << "\t cat <" << cat_it.first.first << "," << cat_it.first.second << "> count "
                   << cat_it.second << " (" << 100.0 * cat_it.second / record_cnt << "%)"
                   << logendl;
    }
}

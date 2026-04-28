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
 
 #pragma once

#include <tbb/tbb.h>
#include <atomic>
#include <future>

#include <exception>

#include "asterixparser.h"
#include "jasterix.h"
#include "logger.h"

namespace jASTERIX
{
class DataBlockFinderTask // : public tbb::task
{
  public:
    DataBlockFinderTask(jASTERIX& jasterix, ASTERIXParser& asterix_parser, const char* data,
                        size_t index, size_t total_size, bool debug)
        : jasterix_(jasterix),
          asterix_parser_(asterix_parser),
          data_(data),
          index_(index),
          total_size_(total_size),
          debug_(debug)
    {
    }

    void start()
    {
        pending_future_ = std::async(std::launch::async, [&] {
            size_t parsed_bytes{0};
            size_t num_data_blocks{0};

            while (!force_stop_ && !done_)  // || size_-index_ > 0
            {
                std::unique_ptr<nlohmann::json> jdata{new nlohmann::json()};

                try
                {
                    std::tuple<size_t, size_t, bool, bool> ret = asterix_parser_.findDataBlocks(
                                data_, index_ + parsed_bytes, total_size_ - parsed_bytes, total_size_,
                                jdata.get(), debug_);

                    parsed_bytes += std::get<0>(ret);
                    num_data_blocks += std::get<1>(ret);
                    if (std::get<2>(ret))
                        error_ = true;
                    done_ = std::get<3>(ret);

//                    loginf << "DataBlockFinderTask: ex pb " << parsed_bytes << " num db "
//                           << num_data_blocks << " done " << done_ << logendl;

                    jasterix_.addDataBlockChunk(std::move(jdata), index_ + parsed_bytes,
                                               error_, done_);
                }
                catch (std::exception& e)
                {
                    error_ = true;
                    done_ = true;

                    // wake consumer parked on data_block_chunks_cv_ so it doesn't deadlock
                    jasterix_.notifyDataBlockChunksError();

                    throw (e);
                }
            }

            if (force_stop_)
                done_ = true;

            // loginf << "data block finder task done " << done << logendl;

        });


    }

    void forceStop();

    bool done() const;

    bool error() const;

  private:
    jASTERIX& jasterix_;
    ASTERIXParser& asterix_parser_;
    const char* data_;
    size_t index_;
    size_t total_size_;
    bool debug_;
    std::atomic<bool> error_{false};
    std::atomic<bool> done_{false};
    std::atomic<bool> force_stop_{false};

    std::future<void> pending_future_;
};

void DataBlockFinderTask::forceStop() { force_stop_ = true; }

bool DataBlockFinderTask::done() const { return done_; }

bool DataBlockFinderTask::error() const { return error_; }

}  // namespace jASTERIX


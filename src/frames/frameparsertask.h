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
#include <tuple>

#include "frameparser.h"
#include "jasterix.h"
#include "logger.h"
#include "traced_assert.h"

namespace jASTERIX
{
class FrameParserTask // : public tbb::task
{
public:
    FrameParserTask(jASTERIX& jasterix, FrameParser& frame_parser, const nlohmann::json& header,
                    const char* data, size_t index, size_t total_size, bool debug)
        : jasterix_(jasterix),
          frame_parser_(frame_parser),
          header_(header),
          data_(data),
          index_(index),
          total_size_(total_size),
          debug_(debug)
    {
        // loginf << "frame parser ctor" << logendl;
    }

    void start()
    {
        pending_future_ = std::async(std::launch::async, [&] {
            if (debug_)
                loginf << "frame parser task execute" << logendl;

            while (!force_stop_ && !done_ && !error_ocurred_)  // || size_-index_ > 0
            {
                std::unique_ptr<nlohmann::json> data_chunk{new nlohmann::json()};

                if (frame_parser_.hasFileHeaderItems())
                    *data_chunk = header_;  // copy header

                traced_assert(index_ < total_size_);

                try
                {
                    std::tuple<size_t, size_t, bool, bool> ret =
                            frame_parser_.findFrames(data_, index_, total_size_, data_chunk.get(), debug_);

                    index_ += std::get<0>(ret);  // parsed bytes
                    done_ = std::get<2>(ret);    // done flag
                    error_ocurred_ = std::get<3>(ret);    // error flag

                    //            if (done_)
                    //                loginf << "frame parser task done" << logendl;

                    traced_assert(data_chunk != nullptr);

                    if (!data_chunk->contains("frames"))
                        throw std::runtime_error("jASTERIX scoped frames information contains no frames");

                    if (!data_chunk->at("frames").is_array())
                        throw std::runtime_error("jASTERIX scoped frames information is not array");

                    jasterix_.addDataChunk(std::move(data_chunk), done_);
                    traced_assert(data_chunk == nullptr);

                }
                catch (std::exception& e)
                {
                    logerr << "frame parser task caught exeception '" << e.what() << "'" << logendl;

                    error_ocurred_ = true;
                    done_ = true;

                    throw (e);
                }
            }

            if (force_stop_)
            {
                done_ = true;
            }

            if (debug_)
                loginf << "frame parser task execute done" << logendl;

        });
    }

    void forceStop();
    bool done() const;
    bool errorOcurred() const { return error_ocurred_; };

private:
    jASTERIX& jasterix_;
    FrameParser& frame_parser_;
    const nlohmann::json& header_;
    const char* data_;
    size_t index_;
    size_t total_size_;
    bool debug_;
    std::atomic<bool> error_ocurred_{false};
    std::atomic<bool> done_{false};
    std::atomic<bool> force_stop_{false};

    std::future<void> pending_future_;
};

void FrameParserTask::forceStop()
{
    //loginf << "frame parser task force stop";
    force_stop_ = true;
}

bool FrameParserTask::done() const { return done_; }

}  // namespace jASTERIX

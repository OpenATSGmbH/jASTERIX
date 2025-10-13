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

#include "jsonwriter.h"

#include <archive.h>
#include <tbb/tbb.h>

#include <chrono>
#include <thread>
#include <vector>

#include "jasterix.h"
#include "jsonfilewritetask.h"
#include "logger.h"
#include "traced_assert.h"

using namespace tbb;
using namespace std;

namespace jASTERIX
{
size_t JSONTextZipFileWriteTask::entry_cnt_ = 0;

JSONWriter::JSONWriter(JSON_OUTPUT_TYPE json_output_type, const std::string& json_path)
    : json_output_type_{json_output_type}, json_path_{json_path}
{
    switch (json_output_type_)
    {
        case JSON_TEXT:
            openJsonFile();
            break;
        case JSON_ZIP_TEXT:
            openJsonZipFile();
            break;
        default:
            throw runtime_error("unhandled JSON output type " + to_string(json_output_type_) +
                                " during construction");
    }
}

JSONWriter::~JSONWriter()
{
    //loginf << "JSONWriter: dtor";

    if (json_data_.size())
        writeData();

    while (file_write_in_progress_)
    {
        // loginf << "JSONWriter: dtor: waiting for file write" << logendl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (json_file_open_)
        closeJsonFile();

    if (json_zip_file_open_)
        closeJsonZipFile();
}

void JSONWriter::write(std::unique_ptr<nlohmann::json> data)
{
    //loginf << "JSONWriter: write: existing json " << json_data_.size();

    switch (json_output_type_)
    {
        case JSON_TEXT:
        case JSON_ZIP_TEXT:
        {
            json_data_.push_back(std::move(data));
        }
        break;
        default:
            throw runtime_error("unhandled JSON output type " + to_string(json_output_type_) +
                                " write");
    }

    if (data_write_size > 0 && json_data_.size() >= static_cast<size_t>(data_write_size))
        writeData();
}

void JSONWriter::writeData()
{
    //loginf << "JSONWriter: writeData: json data size " << json_data_.size();

    traced_assert(json_data_.size());

    for (std::unique_ptr<nlohmann::json>& j_it : json_data_)
        (*j_it)["rec_num"] = rec_num_cnt_++;

    // convert to string
    switch (json_output_type_)
    {
        case JSON_TEXT:
        case JSON_ZIP_TEXT:
            convertJSON2Text();
            break;
        default:
            throw runtime_error("unhandled JSON output type " + to_string(json_output_type_) +
                                " write data");
    }

    traced_assert(!json_data_.size());

    switch (json_output_type_)
    {
        case JSON_TEXT:
            writeTextToFile();
            break;
        case JSON_ZIP_TEXT:
            writeTextToZipFile();
            break;
        default:
            throw runtime_error("unhandled JSON output type " + to_string(json_output_type_) +
                                " write data");
    }

    traced_assert(!text_data_.size());
    //assert(!binary_data_.size());
}

void JSONWriter::convertJSON2Text()
{
    traced_assert(!text_data_.size());
    text_data_.resize(json_data_.size());

    size_t size = json_data_.size();

    tbb::parallel_for(size_t(0), size, [&](size_t cnt) {
        text_data_[cnt] = json_data_.at(cnt)->dump(print_dump_indent) + "\n";
    });

    traced_assert(text_data_.size() == json_data_.size());
    json_data_.clear();
}

void JSONWriter::openJsonFile()
{
    traced_assert(!json_file_open_);

    switch (json_output_type_)
    {
        case JSON_TEXT:
            json_file_.open(json_path_);
            break;
        case JSON_ZIP_TEXT:
            json_file_.open(json_path_, ios::out | ios::binary);
            break;
        default:
            throw runtime_error("unhandled JSON output type " + to_string(json_output_type_) +
                                " open");
    }

    json_file_open_ = true;
}

void JSONWriter::writeTextToFile()
{
    traced_assert(json_file_open_);
    traced_assert(text_data_.size());

    while (file_write_in_progress_)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    file_write_in_progress_ = true;

    // loginf << "JSONWriter: new write task" << logendl;

    std::unique_ptr<JSONTextFileWriteTask> task {
        new JSONTextFileWriteTask(json_file_, std::move(text_data_), *this)};
    task->start();

//    JSONTextFileWriteTask* write_task = new (tbb::task::allocate_root())
//        JSONTextFileWriteTask(json_file_, std::move(text_data_), *this);
//    tbb::task::enqueue(*write_task);

    text_data_.clear();

    traced_assert(!text_data_.size());
}

//void JSONWriter::writeBinaryToFile()
//{
//    traced_assert(json_file_open_);
//    traced_assert(binary_data_.size());

//    //    for (const std::vector<std::uint8_t>& bin_it : binary_data_)
//    //        json_file_.write (reinterpret_cast<const char*>(bin_it.data()), bin_it.size());

//    //    binary_data_.clear();

//    while (file_write_in_progress_)
//        std::this_thread::sleep_for(std::chrono::milliseconds(5));

//    file_write_in_progress_ = true;

//    JSONBinaryFileWriteTask* write_task = new (tbb::task::allocate_root())
//        JSONBinaryFileWriteTask(json_file_, std::move(binary_data_), *this);
//    tbb::task::enqueue(*write_task);

//    binary_data_.clear();

//    traced_assert(!binary_data_.size());
//}

void JSONWriter::closeJsonFile()
{
    json_file_.close();
    json_file_open_ = false;
}

void JSONWriter::openJsonZipFile()
{
    traced_assert(!json_zip_file_open_);

    json_zip_file_ = archive_write_new();
    archive_write_set_format_zip(json_zip_file_);
    archive_write_open_filename(json_zip_file_, json_path_.c_str());

    json_zip_file_open_ = true;
}

void JSONWriter::writeTextToZipFile()
{
    traced_assert(json_zip_file_open_);
    traced_assert(text_data_.size());

    while (file_write_in_progress_)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    file_write_in_progress_ = true;

    std::unique_ptr<JSONTextZipFileWriteTask> task {
        new JSONTextZipFileWriteTask(json_zip_file_, std::move(text_data_), *this)};
    task->start();

//    JSONTextZipFileWriteTask* write_task = new (tbb::task::allocate_root())
//        JSONTextZipFileWriteTask(json_zip_file_, std::move(text_data_), *this);
//    tbb::task::enqueue(*write_task);

    text_data_.clear();
}

//void JSONWriter::writeBinaryToZipFile()
//{
//    traced_assert(json_zip_file_open_);
//    traced_assert(binary_data_.size());

//    //    for (const std::vector<std::uint8_t> bin_it : binary_data_)
//    //        archive_write_data (json_zip_file_, reinterpret_cast<const void*>(bin_it.data()),
//    //        bin_it.size());

//    //    binary_data_.clear();

//    while (file_write_in_progress_)
//        std::this_thread::sleep_for(std::chrono::milliseconds(5));

//    file_write_in_progress_ = true;

//    JSONBinaryZipFileWriteTask* write_task = new (tbb::task::allocate_root())
//        JSONBinaryZipFileWriteTask(json_zip_file_, std::move(binary_data_), *this);
//    tbb::task::enqueue(*write_task);

//    traced_assert(!binary_data_.size());
//}

void JSONWriter::closeJsonZipFile()
{
    archive_write_close(json_zip_file_);  // Note 4
    archive_write_free(json_zip_file_);   // Note 5

    json_zip_file_open_ = false;
}

void JSONWriter::fileWritingDone()
{
    // loginf << "JSONWriter: fileWritingDone" << logendl;
    file_write_in_progress_ = false;
}

}  // namespace jASTERIX

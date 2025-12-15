// MIT License
//
// Copyright (c) 2025 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "core/trace_cache/buffer_storage.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <signal.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

class flush_worker_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_file_path =
            "flush_test_" + std::to_string(test_counter.fetch_add(1)) + ".bin";
        std::remove(test_file_path.c_str());
        worker_sync =
            std::make_shared<rocprofsys::trace_cache::worker_synchronization_t>();
    }

    void TearDown() override { std::remove(test_file_path.c_str()); }

    rocprofsys::trace_cache::worker_synchronization_ptr_t worker_sync;
    std::string                                           test_file_path;
    static std::atomic<int>                               test_counter;
};

std::atomic<int> flush_worker_test::test_counter{ 0 };

namespace
{
size_t
get_file_size(const std::string& path)
{
    struct stat st;
    if(stat(path.c_str(), &st) == 0) return static_cast<size_t>(st.st_size);
    return 0;
}

std::vector<uint8_t>
read_file_contents(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if(!file) return {};
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),
                                std::istreambuf_iterator<char>());
}
}  // namespace

TEST_F(flush_worker_test, start_worker_in_correct_state)
{
    bool worker_called   = false;
    auto worker_function = [&](rocprofsys::trace_cache::ofs_t&, bool) {
        worker_called = true;
    };

    rocprofsys::trace_cache::flush_worker_t worker(worker_function, worker_sync,
                                                   test_file_path);
    pid_t                                   current_pid = getpid();

    worker.start(current_pid);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_TRUE(worker_sync->is_running);
    EXPECT_EQ(worker_sync->origin_pid, current_pid);

    worker.stop(current_pid);
}

TEST_F(flush_worker_test, stop_worker_complete)
{
    std::atomic<bool> worker_called{ false };
    auto              worker_function = [&](rocprofsys::trace_cache::ofs_t&, bool) {
        worker_called = true;
    };

    rocprofsys::trace_cache::flush_worker_t worker(worker_function, worker_sync,
                                                   test_file_path);
    pid_t                                   current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    worker.stop(current_pid);

    EXPECT_TRUE(worker_sync->exit_finished);
    EXPECT_FALSE(worker_sync->is_running);
    EXPECT_TRUE(worker_called);
}

TEST_F(flush_worker_test, worker_function_called_on_stop)
{
    std::atomic<int>  call_count{ 0 };
    std::atomic<bool> force_flag{ false };
    auto              worker_function = [&](rocprofsys::trace_cache::ofs_t&, bool force) {
        call_count++;
        force_flag = force;
    };

    rocprofsys::trace_cache::flush_worker_t worker(worker_function, worker_sync,
                                                   test_file_path);
    pid_t                                   current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    worker.stop(current_pid);

    EXPECT_GE(call_count.load(), 1);
    EXPECT_TRUE(force_flag);
}

TEST_F(flush_worker_test, multiple_stop_calls_are_safe)
{
    auto worker_function = [](rocprofsys::trace_cache::ofs_t&, bool) {};

    rocprofsys::trace_cache::flush_worker_t worker(worker_function, worker_sync,
                                                   test_file_path);
    pid_t                                   current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    worker.stop(current_pid);
    worker.stop(current_pid);
    worker.stop(current_pid);

    EXPECT_TRUE(worker_sync->exit_finished);
    EXPECT_FALSE(worker_sync->is_running);
}

TEST_F(flush_worker_test, worker_factory_creates_valid_object)
{
    auto worker_function = [](rocprofsys::trace_cache::ofs_t&, bool) {};

    auto worker = rocprofsys::trace_cache::flush_worker_factory_t::get_worker(
        worker_function, worker_sync, test_file_path);

    EXPECT_NE(worker, nullptr);
    EXPECT_EQ(typeid(*worker), typeid(rocprofsys::trace_cache::flush_worker_t));
}

TEST_F(flush_worker_test, worker_handles_invalid_path)
{
    auto        worker_function = [](rocprofsys::trace_cache::ofs_t&, bool) {};
    std::string invalid_path    = "/invalid/path/file.bin";

    rocprofsys::trace_cache::flush_worker_t worker(worker_function, worker_sync,
                                                   invalid_path);
    pid_t                                   current_pid = getpid();

    EXPECT_THROW(worker.start(current_pid), std::runtime_error);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    worker.stop(current_pid);

    EXPECT_FALSE(worker_sync->exit_finished);
    EXPECT_FALSE(worker_sync->is_running);
}

TEST_F(flush_worker_test, different_pid_start_stop)
{
    std::atomic<bool> worker_called{ false };
    auto              worker_function = [&](rocprofsys::trace_cache::ofs_t&, bool) {
        worker_called = true;
    };

    rocprofsys::trace_cache::flush_worker_t worker(worker_function, worker_sync,
                                                   test_file_path);
    pid_t                                   parent_pid = getpid();

    worker.start(parent_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_TRUE(worker_sync->is_running);
    EXPECT_EQ(worker_sync->origin_pid, parent_pid);

    pid_t child_pid = fork();
    if(child_pid == 0)
    {
        pid_t current_child_pid = getpid();
        worker.stop(current_child_pid);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        bool still_running = worker_sync->is_running;
        bool exit_finished = worker_sync->exit_finished;

        exit(still_running ? 1 : (exit_finished ? 2 : 0));
    }
    else
    {
        int status;
        waitpid(child_pid, &status, 0);
        int child_exit_code = WEXITSTATUS(status);

        EXPECT_EQ(child_exit_code, 0);
        EXPECT_FALSE(worker_sync->exit_finished);
        EXPECT_TRUE(worker_sync->is_running);

        worker.stop(parent_pid);
        EXPECT_TRUE(worker_sync->exit_finished);
        EXPECT_FALSE(worker_sync->is_running);
        EXPECT_TRUE(worker_called);
    }
}

TEST_F(flush_worker_test, data_lost_on_sigkill)
{
    const std::string test_marker     = "FLUSH_WORKER_SIGKILL_TEST_DATA_MARKER_12345";
    const size_t      expected_writes = 10;
    const std::string sigkill_test_file =
        "/tmp/flush_worker_sigkill_test_" + std::to_string(getpid()) + ".bin";

    std::remove(sigkill_test_file.c_str());

    pid_t child_pid = fork();
    if(child_pid == 0)
    {
        auto worker_sync_child =
            std::make_shared<rocprofsys::trace_cache::worker_synchronization_t>();

        size_t write_count     = 0;
        auto   worker_function = [&](rocprofsys::trace_cache::ofs_t& ofs, bool) {
            if(write_count < expected_writes)
            {
                ofs.write(test_marker.data(),
                            static_cast<std::streamsize>(test_marker.size()));
                write_count++;
            }
        };

        rocprofsys::trace_cache::flush_worker_t worker(worker_function, worker_sync_child,
                                                       sigkill_test_file);

        worker.start(getpid());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        kill(getpid(), SIGKILL);

        _exit(1);
    }
    else
    {
        int status;
        waitpid(child_pid, &status, 0);

        EXPECT_TRUE(WIFSIGNALED(status));
        EXPECT_EQ(WTERMSIG(status), SIGKILL);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        auto   contents  = read_file_contents(sigkill_test_file);
        size_t file_size = get_file_size(sigkill_test_file);

        size_t markers_found = 0;
        if(!contents.empty())
        {
            std::string content_str(contents.begin(), contents.end());
            size_t      pos = 0;
            while((pos = content_str.find(test_marker, pos)) != std::string::npos)
            {
                markers_found++;
                pos += test_marker.size();
            }
        }

        std::remove(sigkill_test_file.c_str());

        EXPECT_LT(markers_found, expected_writes)
            << "Expected data loss on SIGKILL for ofstream-based flush_worker. "
            << "Found " << markers_found << " markers, expected less than "
            << expected_writes << ". File size: " << file_size;
    }
}

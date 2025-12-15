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

class mmap_flush_worker_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_file_path =
            "/tmp/mmap_flush_test_" + std::to_string(test_counter.fetch_add(1)) + ".bin";
        std::remove(test_file_path.c_str());
        worker_sync =
            std::make_shared<rocprofsys::trace_cache::worker_synchronization_t>();
    }

    void TearDown() override { std::remove(test_file_path.c_str()); }

    rocprofsys::trace_cache::worker_synchronization_ptr_t worker_sync;
    std::string                                           test_file_path;
    static std::atomic<int>                               test_counter;

    static size_t get_file_size(const std::string& path)
    {
        struct stat st;
        if(stat(path.c_str(), &st) == 0) return static_cast<size_t>(st.st_size);
        return 0;
    }

    static std::vector<uint8_t> read_file_contents(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if(!file) return {};
        return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),
                                    std::istreambuf_iterator<char>());
    }
};

std::atomic<int> mmap_flush_worker_test::test_counter{ 0 };

TEST_F(mmap_flush_worker_test, start_worker_in_correct_state)
{
    std::atomic<bool> worker_called{ false };
    auto flush_callback = [&](uint8_t*, size_t, size_t& bytes_written, bool) {
        worker_called = true;
        bytes_written = 0;
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_TRUE(worker_sync->is_running);
    EXPECT_EQ(worker_sync->origin_pid, current_pid);

    worker.stop(current_pid);
}

TEST_F(mmap_flush_worker_test, stop_worker_complete)
{
    std::atomic<bool> worker_called{ false };
    auto flush_callback = [&](uint8_t*, size_t, size_t& bytes_written, bool) {
        worker_called = true;
        bytes_written = 0;
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    worker.stop(current_pid);

    EXPECT_TRUE(worker_sync->exit_finished);
    EXPECT_FALSE(worker_sync->is_running);
    EXPECT_TRUE(worker_called);
}

TEST_F(mmap_flush_worker_test, callback_invoked_with_force_on_stop)
{
    std::atomic<int>  call_count{ 0 };
    std::atomic<bool> force_flag{ false };
    auto flush_callback = [&](uint8_t*, size_t, size_t& bytes_written, bool force) {
        call_count++;
        force_flag    = force;
        bytes_written = 0;
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    worker.stop(current_pid);

    EXPECT_GE(call_count.load(), 1);
    EXPECT_TRUE(force_flag);
}

TEST_F(mmap_flush_worker_test, multiple_stop_calls_are_safe)
{
    auto flush_callback = [](uint8_t*, size_t, size_t& bytes_written, bool) {
        bytes_written = 0;
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    worker.stop(current_pid);
    worker.stop(current_pid);
    worker.stop(current_pid);

    EXPECT_TRUE(worker_sync->exit_finished);
    EXPECT_FALSE(worker_sync->is_running);
}

TEST_F(mmap_flush_worker_test, worker_factory_creates_valid_object)
{
    auto flush_callback = [](uint8_t*, size_t, size_t& bytes_written, bool) {
        bytes_written = 0;
    };

    auto worker = rocprofsys::trace_cache::mmap_flush_worker_factory_t<>::get_worker(
        flush_callback, worker_sync, test_file_path);

    EXPECT_NE(worker, nullptr);
    EXPECT_EQ(typeid(*worker), typeid(rocprofsys::trace_cache::mmap_flush_worker_t));
}

TEST_F(mmap_flush_worker_test, worker_handles_invalid_path)
{
    auto flush_callback = [](uint8_t*, size_t, size_t& bytes_written, bool) {
        bytes_written = 0;
    };
    std::string invalid_path = "/invalid/nonexistent/path/file.bin";

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        invalid_path, max_file_size);
    pid_t                                        current_pid = getpid();

    EXPECT_THROW(worker.start(current_pid), std::runtime_error);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    worker.stop(current_pid);

    EXPECT_FALSE(worker_sync->exit_finished);
    EXPECT_FALSE(worker_sync->is_running);
}

TEST_F(mmap_flush_worker_test, data_persisted_to_file)
{
    const std::string test_data = "Hello, mmap flush worker!";
    auto flush_callback = [&](uint8_t* dest, size_t max_bytes, size_t& bytes_written,
                              bool force) {
        if(force && max_bytes >= test_data.size())
        {
            std::memcpy(dest, test_data.data(), test_data.size());
            bytes_written = test_data.size();
        }
        else
        {
            bytes_written = 0;
        }
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    worker.stop(current_pid);

    auto contents = read_file_contents(test_file_path);
    ASSERT_EQ(contents.size(), test_data.size());
    EXPECT_EQ(std::string(contents.begin(), contents.end()), test_data);
}

TEST_F(mmap_flush_worker_test, file_truncated_to_actual_size)
{
    size_t total_written  = 0;
    auto   flush_callback = [&](uint8_t* dest, size_t max_bytes, size_t& bytes_written,
                              bool) {
        const size_t write_size = 100;
        if(max_bytes >= write_size && total_written < 500)
        {
            std::memset(dest, 'X', write_size);
            bytes_written = write_size;
            total_written += write_size;
        }
        else
        {
            bytes_written = 0;
        }
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    worker.stop(current_pid);

    size_t final_size = get_file_size(test_file_path);
    EXPECT_EQ(final_size, total_written);
    EXPECT_LT(final_size, max_file_size);
}

TEST_F(mmap_flush_worker_test, max_bytes_respected)
{
    std::atomic<size_t> reported_max_bytes{ 0 };
    auto flush_callback = [&](uint8_t*, size_t max_bytes, size_t& bytes_written, bool) {
        reported_max_bytes = max_bytes;
        bytes_written      = 0;
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    worker.stop(current_pid);

    EXPECT_GT(reported_max_bytes.load(), 0u);
    EXPECT_LE(reported_max_bytes.load(),
              rocprofsys::trace_cache::mmap_flush_worker_t::initial_mmap_size);
}

TEST_F(mmap_flush_worker_test, dynamic_resize_triggers)
{
    size_t           write_offset = 0;
    std::atomic<int> flush_count{ 0 };
    constexpr size_t initial_size =
        rocprofsys::trace_cache::mmap_flush_worker_t::initial_mmap_size;
    constexpr size_t chunk_size = initial_size / 4;

    auto flush_callback = [&](uint8_t* dest, size_t max_bytes, size_t& bytes_written,
                              bool) {
        flush_count++;
        if(max_bytes >= chunk_size)
        {
            std::memset(dest, 'A', chunk_size);
            bytes_written = chunk_size;
            write_offset += chunk_size;
        }
        else
        {
            bytes_written = 0;
        }
    };

    constexpr size_t                             max_file_size = 256 * 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);

    while(write_offset < initial_size * 3 / 4)
    {
        worker_sync->notify_data_ready();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    worker.stop(current_pid);

    size_t final_size = get_file_size(test_file_path);
    EXPECT_EQ(final_size, write_offset);
    EXPECT_GT(write_offset, initial_size * 3 / 4);
}

TEST_F(mmap_flush_worker_test, different_pid_start_stop)
{
    std::atomic<bool> worker_called{ false };
    auto flush_callback = [&](uint8_t*, size_t, size_t& bytes_written, bool) {
        worker_called = true;
        bytes_written = 0;
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        parent_pid = getpid();

    worker.start(parent_pid);
    worker_sync->notify_data_ready();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

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

        _exit(still_running ? 1 : (exit_finished ? 2 : 0));
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

TEST_F(mmap_flush_worker_test, large_data_write)
{
    std::atomic<size_t> total_written{ 0 };
    constexpr size_t    chunk_size  = 1024 * 1024;
    constexpr size_t    target_size = 10 * 1024 * 1024;

    auto flush_callback = [&](uint8_t* dest, size_t max_bytes, size_t& bytes_written,
                              bool) {
        if(total_written.load() < target_size && max_bytes >= chunk_size)
        {
            std::memset(dest, 'D', chunk_size);
            bytes_written = chunk_size;
            total_written += chunk_size;
        }
        else
        {
            bytes_written = 0;
        }
    };

    constexpr size_t                             max_file_size = 64 * 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);

    auto start = std::chrono::steady_clock::now();
    while(total_written.load() < target_size)
    {
        worker_sync->notify_data_ready();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto elapsed = std::chrono::steady_clock::now() - start;
        if(elapsed > std::chrono::seconds(5))
        {
            break;
        }
    }

    worker.stop(current_pid);

    size_t final_size = get_file_size(test_file_path);
    EXPECT_GE(final_size, target_size);

    auto contents = read_file_contents(test_file_path);
    EXPECT_EQ(contents.size(), final_size);
    for(size_t i = 0; i < std::min(contents.size(), size_t(100)); i++)
    {
        EXPECT_EQ(contents[i], 'D');
    }
}

TEST_F(mmap_flush_worker_test, small_max_file_size)
{
    std::atomic<size_t> total_written{ 0 };
    constexpr size_t    chunk_size    = 1024;
    constexpr size_t    max_file_size = 4096;

    auto flush_callback = [&](uint8_t* dest, size_t max_bytes, size_t& bytes_written,
                              bool) {
        if(max_bytes >= chunk_size)
        {
            std::memset(dest, 'S', chunk_size);
            bytes_written = chunk_size;
            total_written += chunk_size;
        }
        else
        {
            bytes_written = 0;
        }
    };

    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);

    for(int i = 0; i < 10; i++)
    {
        worker_sync->notify_data_ready();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    worker.stop(current_pid);

    size_t final_size = get_file_size(test_file_path);
    EXPECT_LE(final_size, max_file_size);
    EXPECT_EQ(final_size, total_written.load());
}

TEST_F(mmap_flush_worker_test, event_driven_flush)
{
    std::atomic<int> call_count{ 0 };

    auto flush_callback = [&](uint8_t*, size_t, size_t& bytes_written, bool) {
        call_count++;
        bytes_written = 0;
    };

    constexpr size_t                             max_file_size = 1024 * 1024;
    rocprofsys::trace_cache::mmap_flush_worker_t worker(flush_callback, worker_sync,
                                                        test_file_path, max_file_size);
    pid_t                                        current_pid = getpid();

    worker.start(current_pid);

    constexpr int num_notifications = 10;
    for(int i = 0; i < num_notifications; i++)
    {
        worker_sync->notify_data_ready();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    worker.stop(current_pid);

    EXPECT_GE(call_count.load(), num_notifications);
}

TEST_F(mmap_flush_worker_test, data_survives_sigkill)
{
    const std::string test_marker     = "MMAP_WORKER_SIGKILL_TEST_DATA_MARKER_12345";
    const size_t      expected_writes = 10;
    const std::string sigkill_test_file =
        "/tmp/mmap_worker_sigkill_test_" + std::to_string(getpid()) + ".bin";

    std::remove(sigkill_test_file.c_str());

    pid_t child_pid = fork();
    if(child_pid == 0)
    {
        auto worker_sync_child =
            std::make_shared<rocprofsys::trace_cache::worker_synchronization_t>();

        size_t write_count  = 0;
        auto flush_callback = [&](uint8_t* dest, size_t max_bytes, size_t& bytes_written,
                                  bool) {
            if(write_count < expected_writes && max_bytes >= test_marker.size())
            {
                std::memcpy(dest, test_marker.data(), test_marker.size());
                bytes_written = test_marker.size();
                write_count++;
            }
            else
            {
                bytes_written = 0;
            }
        };

        constexpr size_t                             max_file_size = 1024 * 1024;
        rocprofsys::trace_cache::mmap_flush_worker_t worker(
            flush_callback, worker_sync_child, sigkill_test_file, max_file_size);

        worker.start(getpid());

        while(write_count < expected_writes)
        {
            worker_sync_child->notify_data_ready();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

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

        EXPECT_EQ(markers_found, expected_writes)
            << "Expected ALL data to survive SIGKILL for mmap-based worker. " << "Found "
            << markers_found << " markers out of " << expected_writes
            << " expected. File size: " << file_size;
    }
}

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

#include "core/trace_cache/metadata_registry.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace rocprofsys::trace_cache;

struct mock_buffer_storage
{
    explicit mock_buffer_storage(std::string filepath)
    : m_filepath(std::move(filepath))
    {}

    void start(int32_t pid)
    {
        m_running     = true;
        m_started     = true;
        m_current_pid = pid;
    }

    void shutdown()
    {
        m_running  = false;
        m_shutdown = true;
    }

    bool is_running() const { return m_running; }

    template <typename T>
    void store(const T& value)
    {
        m_store_count++;
        m_last_stored_type_id = static_cast<uint8_t>(T::type_identifier);
    }

    std::string m_filepath;
    bool        m_running{ false };
    bool        m_started{ false };
    bool        m_shutdown{ false };
    int32_t     m_current_pid{ 0 };
    size_t      m_store_count{ 0 };
    uint8_t     m_last_stored_type_id{ 0 };
};

using mock_registry_t = metadata_registry_t<mock_buffer_storage>;

class metadata_registry_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_registry = std::make_unique<mock_registry_t>("/tmp/test_metadata.bin");
    }

    void TearDown() override { m_registry.reset(); }

    mock_buffer_storage& get_buffer()
    {
        return *reinterpret_cast<mock_buffer_storage*>(m_registry.get());
    }

    std::unique_ptr<mock_registry_t> m_registry;
};

TEST_F(metadata_registry_test, construction)
{
    EXPECT_FALSE(get_buffer().m_running);
    EXPECT_FALSE(get_buffer().m_started);
    EXPECT_EQ(get_buffer().m_filepath, "/tmp/test_metadata.bin");
}

TEST_F(metadata_registry_test, start_and_shutdown)
{
    m_registry->start(1234);
    EXPECT_TRUE(get_buffer().m_running);
    EXPECT_TRUE(get_buffer().m_started);
    EXPECT_EQ(get_buffer().m_current_pid, 1234);

    m_registry->shutdown();
    EXPECT_FALSE(get_buffer().m_running);
    EXPECT_TRUE(get_buffer().m_shutdown);
}

TEST_F(metadata_registry_test, set_process_does_not_store_to_buffer)
{
    m_registry->start(1234);

    info::process proc{ 100, 200, "/usr/bin/app" };
    m_registry->set_process(proc);

    EXPECT_EQ(get_buffer().m_store_count, 0);
}

TEST_F(metadata_registry_test, set_process_start_time_does_not_store_to_buffer)
{
    m_registry->start(1234);

    info::process_start_time start_time{ 1000000000 };
    m_registry->set_process_start_time(start_time);

    EXPECT_EQ(get_buffer().m_store_count, 0);
}

TEST_F(metadata_registry_test, set_process_end_time_does_not_store_to_buffer)
{
    m_registry->start(1234);

    info::process_end_time end_time{ 2000000000 };
    m_registry->set_process_end_time(end_time);

    EXPECT_EQ(get_buffer().m_store_count, 0);
}

TEST_F(metadata_registry_test, add_pmc_stores_to_buffer)
{
    m_registry->start(1234);

    info::pmc pmc{ rocprofsys::agent_type::GPU,
                   0,
                   "gfx90a",
                   100,
                   0,
                   "counter",
                   "C1",
                   "desc",
                   "ld",
                   "comp",
                   "cycles",
                   "u64",
                   "blk",
                   "",
                   0,
                   0 };
    m_registry->add_pmc_info(pmc);

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::pmc));
}

TEST_F(metadata_registry_test, add_pmc_deduplicates)
{
    m_registry->start(1234);

    info::pmc pmc1{ rocprofsys::agent_type::GPU,
                    0,
                    "gfx90a",
                    100,
                    0,
                    "counter",
                    "C1",
                    "desc",
                    "ld",
                    "comp",
                    "cycles",
                    "u64",
                    "blk",
                    "",
                    0,
                    0 };
    info::pmc pmc2{ rocprofsys::agent_type::GPU,
                    0,
                    "gfx90a",
                    100,
                    0,
                    "counter",
                    "C1",
                    "desc",
                    "ld",
                    "comp",
                    "cycles",
                    "u64",
                    "blk",
                    "",
                    0,
                    0 };

    m_registry->add_pmc_info(pmc1);
    m_registry->add_pmc_info(pmc2);

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, add_different_pmc_stores_both)
{
    m_registry->start(1234);

    info::pmc pmc1{ rocprofsys::agent_type::GPU,
                    0,
                    "gfx90a",
                    100,
                    0,
                    "counter1",
                    "C1",
                    "desc",
                    "ld",
                    "comp",
                    "cycles",
                    "u64",
                    "blk",
                    "",
                    0,
                    0 };
    info::pmc pmc2{ rocprofsys::agent_type::CPU,
                    1,
                    "zen3",
                    200,
                    1,
                    "counter2",
                    "C2",
                    "desc",
                    "ld",
                    "comp",
                    "bytes",
                    "u64",
                    "blk",
                    "",
                    0,
                    0 };

    m_registry->add_pmc_info(pmc1);
    m_registry->add_pmc_info(pmc2);

    EXPECT_EQ(get_buffer().m_store_count, 2);
}

TEST_F(metadata_registry_test, add_thread_stores_to_buffer)
{
    m_registry->start(1234);

    info::thread thread{ 100, 1234, 9999, 0, 1000, "{}" };
    m_registry->add_thread_info(thread);

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::thread));
}

TEST_F(metadata_registry_test, add_thread_deduplicates)
{
    m_registry->start(1234);

    info::thread thread1{ 100, 1234, 9999, 0, 1000, "{}" };
    info::thread thread2{ 200, 5678, 9999, 100, 2000, "{}" };

    m_registry->add_thread_info(thread1);
    m_registry->add_thread_info(thread2);

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, add_track_stores_to_buffer)
{
    m_registry->start(1234);

    info::track track{ "GPU Track", std::make_optional<size_t>(1001), "{}" };
    m_registry->add_track(track);

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::track));
}

TEST_F(metadata_registry_test, add_track_deduplicates)
{
    m_registry->start(1234);

    info::track track1{ "GPU Track", std::make_optional<size_t>(1001), "{}" };
    info::track track2{ "GPU Track", std::nullopt, "{\"other\":true}" };

    m_registry->add_track(track1);
    m_registry->add_track(track2);

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, add_queue_stores_to_buffer)
{
    m_registry->start(1234);

    m_registry->add_queue(0xABCD1234);

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::queue));
}

TEST_F(metadata_registry_test, add_queue_deduplicates)
{
    m_registry->start(1234);

    m_registry->add_queue(0xABCD1234);
    m_registry->add_queue(0xABCD1234);

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, add_stream_stores_to_buffer)
{
    m_registry->start(1234);

    m_registry->add_stream(0x1111);

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::stream));
}

TEST_F(metadata_registry_test, add_stream_deduplicates)
{
    m_registry->start(1234);

    m_registry->add_stream(0x1111);
    m_registry->add_stream(0x1111);

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, add_string_stores_to_buffer)
{
    m_registry->start(1234);

    m_registry->add_string("test_string");

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::string));
}

TEST_F(metadata_registry_test, add_string_deduplicates)
{
    m_registry->start(1234);

    m_registry->add_string("test_string");
    m_registry->add_string("test_string");

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, add_agent_stores_to_buffer)
{
    m_registry->start(1234);

    auto agent_ptr               = std::make_shared<rocprofsys::agent>();
    agent_ptr->type              = rocprofsys::agent_type::GPU;
    agent_ptr->device_id         = 0;
    agent_ptr->device_type_index = 0;

    info::agent_t agent_info{ agent_ptr };
    m_registry->add_agent_info(agent_info);

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::agent));
}

TEST_F(metadata_registry_test, add_agent_deduplicates)
{
    m_registry->start(1234);

    auto agent_ptr1               = std::make_shared<rocprofsys::agent>();
    agent_ptr1->type              = rocprofsys::agent_type::GPU;
    agent_ptr1->device_id         = 0;
    agent_ptr1->device_type_index = 0;

    auto agent_ptr2               = std::make_shared<rocprofsys::agent>();
    agent_ptr2->type              = rocprofsys::agent_type::GPU;
    agent_ptr2->device_id         = 0;
    agent_ptr2->device_type_index = 0;

    info::agent_t agent_info1{ agent_ptr1 };
    info::agent_t agent_info2{ agent_ptr2 };

    m_registry->add_agent_info(agent_info1);
    m_registry->add_agent_info(agent_info2);

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, add_code_object_stores_to_buffer)
{
    m_registry->start(1234);

    info::code_object co{ 1, "file:///path/to/code.so", 0x1000, 0x500, 0, 0, 0 };
    m_registry->add_code_object(co);

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::code_object));
}

TEST_F(metadata_registry_test, add_code_object_deduplicates)
{
    m_registry->start(1234);

    info::code_object co1{ 1, "file:///path/to/code1.so", 0x1000, 0x500, 0, 0, 0 };
    info::code_object co2{ 1, "file:///path/to/code2.so", 0x2000, 0x600, 100, 1, 1 };

    m_registry->add_code_object(co1);
    m_registry->add_code_object(co2);

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, add_kernel_symbol_stores_to_buffer)
{
    m_registry->start(1234);

    info::kernel_symbol ks{ 100, 1, "kernel_a", 0x3000, 256, 8, 1024, 512, 32, 64, 0 };
    m_registry->add_kernel_symbol(ks);

    EXPECT_EQ(get_buffer().m_store_count, 1);
    EXPECT_EQ(get_buffer().m_last_stored_type_id,
              static_cast<uint8_t>(info::metadata_identifier_t::kernel_symbol));
}

TEST_F(metadata_registry_test, add_kernel_symbol_deduplicates)
{
    m_registry->start(1234);

    info::kernel_symbol ks1{ 100, 1, "kernel_a", 0x3000, 256, 8, 1024, 512, 32, 64, 0 };
    info::kernel_symbol ks2{
        100, 2, "kernel_b", 0x4000, 512, 16, 2048, 1024, 64, 128, 16
    };

    m_registry->add_kernel_symbol(ks1);
    m_registry->add_kernel_symbol(ks2);

    EXPECT_EQ(get_buffer().m_store_count, 1);
}

TEST_F(metadata_registry_test, does_not_store_when_not_running)
{
    info::pmc pmc{ rocprofsys::agent_type::GPU,
                   0,
                   "gfx90a",
                   100,
                   0,
                   "counter",
                   "C1",
                   "desc",
                   "ld",
                   "comp",
                   "cycles",
                   "u64",
                   "blk",
                   "",
                   0,
                   0 };
    m_registry->add_pmc_info(pmc);

    EXPECT_EQ(get_buffer().m_store_count, 0);
}

TEST_F(metadata_registry_test, multiple_types_stored)
{
    m_registry->start(1234);

    info::pmc    pmc{ rocprofsys::agent_type::GPU,
                   0,
                   "gfx90a",
                   100,
                   0,
                   "counter",
                   "C1",
                   "desc",
                   "ld",
                   "comp",
                   "cycles",
                   "u64",
                   "blk",
                   "",
                   0,
                   0 };
    info::thread thread{ 100, 1234, 9999, 0, 1000, "{}" };
    info::track  track{ "GPU Track", std::nullopt, "{}" };

    m_registry->add_pmc_info(pmc);
    m_registry->add_thread_info(thread);
    m_registry->add_track(track);
    m_registry->add_queue(0x1234);
    m_registry->add_stream(0x5678);
    m_registry->add_string("test");

    EXPECT_EQ(get_buffer().m_store_count, 6);
}

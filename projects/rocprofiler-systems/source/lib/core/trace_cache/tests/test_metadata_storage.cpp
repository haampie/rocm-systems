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

#include "core/trace_cache/metadata_storage.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

using namespace rocprofsys::trace_cache;

class metadata_parser_output_test : public ::testing::Test
{
protected:
    void SetUp() override { m_storage = std::make_shared<metadata_parser_output_t>(); }

    std::shared_ptr<metadata_parser_output_t> m_storage;
};

TEST_F(metadata_parser_output_test, set_and_get_process_info)
{
    info::process proc{ 1234, 5678, "/usr/bin/test_app" };
    m_storage->set_process(proc);

    auto result = m_storage->get_process_info();
    EXPECT_EQ(result.pid, 1234);
    EXPECT_EQ(result.ppid, 5678);
    EXPECT_EQ(result.command, "/usr/bin/test_app");
}

TEST_F(metadata_parser_output_test, set_and_get_process_start_time)
{
    info::process_start_time start_time{ 1000000000 };
    m_storage->set_process_start_time(start_time);

    auto result = m_storage->get_process_start_time();
    EXPECT_EQ(result.start, 1000000000);
}

TEST_F(metadata_parser_output_test, set_and_get_process_end_time)
{
    info::process_end_time end_time{ 2000000000 };
    m_storage->set_process_end_time(end_time);

    auto result = m_storage->get_process_end_time();
    EXPECT_EQ(result.end, 2000000000);
}

TEST_F(metadata_parser_output_test, add_and_get_pmc_info)
{
    info::pmc pmc1{ rocprofsys::agent_type::GPU,
                    0,
                    "gfx90a",
                    100,
                    0,
                    "counter1",
                    "C1",
                    "desc1",
                    "ld",
                    "comp1",
                    "cycles",
                    "u64",
                    "blk1",
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
                    "desc2",
                    "ld",
                    "comp2",
                    "bytes",
                    "u64",
                    "blk2",
                    "",
                    1,
                    1 };

    m_storage->add_pmc_info(pmc1);
    m_storage->add_pmc_info(pmc2);

    auto result = m_storage->get_pmc_info_list();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].name, "counter1");
    EXPECT_EQ(result[0].type, rocprofsys::agent_type::GPU);
    EXPECT_EQ(result[1].name, "counter2");
    EXPECT_EQ(result[1].type, rocprofsys::agent_type::CPU);
}

TEST_F(metadata_parser_output_test, add_and_get_thread_info)
{
    info::thread thread1{ 100, 1234, 1001, 0, 100, "{}" };
    info::thread thread2{ 100, 1234, 1002, 10, 200, "{}" };

    m_storage->add_thread_info(thread1);
    m_storage->add_thread_info(thread2);

    auto result = m_storage->get_thread_info_list();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].thread_id, 1001);
    EXPECT_EQ(result[1].thread_id, 1002);
}

TEST_F(metadata_parser_output_test, add_and_get_track_info)
{
    info::track track1{ "GPU Track 0", std::make_optional<size_t>(1001), "{}" };
    info::track track2{ "CPU Track", std::nullopt, "{}" };

    m_storage->add_track(track1);
    m_storage->add_track(track2);

    auto result = m_storage->get_track_info_list();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].track_name, "GPU Track 0");
    EXPECT_TRUE(result[0].thread_id.has_value());
    EXPECT_EQ(result[0].thread_id.value(), 1001);
    EXPECT_EQ(result[1].track_name, "CPU Track");
    EXPECT_FALSE(result[1].thread_id.has_value());
}

TEST_F(metadata_parser_output_test, add_and_get_queue_info)
{
    info::queue queue1{ 0xABCD1234 };
    info::queue queue2{ 0xDEADBEEF };

    m_storage->add_queue(queue1);
    m_storage->add_queue(queue2);

    auto result = m_storage->get_queue_list();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].handle, 0xABCD1234);
    EXPECT_EQ(result[1].handle, 0xDEADBEEF);
}

TEST_F(metadata_parser_output_test, add_and_get_stream_info)
{
    info::stream stream1{ 0x1111 };
    info::stream stream2{ 0x2222 };

    m_storage->add_stream(stream1);
    m_storage->add_stream(stream2);

    auto result = m_storage->get_stream_list();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].handle, 0x1111);
    EXPECT_EQ(result[1].handle, 0x2222);
}

TEST_F(metadata_parser_output_test, add_and_get_strings)
{
    std::string        str1 = "first_string";
    std::string        str2 = "second_string";
    info::string_entry entry1{ str1 };
    info::string_entry entry2{ str2 };

    m_storage->add_string(entry1);
    m_storage->add_string(entry2);

    auto result = m_storage->get_string_list();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "first_string");
    EXPECT_EQ(result[1], "second_string");
}

TEST_F(metadata_parser_output_test, add_and_get_agent_info)
{
    auto agent1        = std::make_shared<rocprofsys::agent>();
    agent1->type       = rocprofsys::agent_type::GPU;
    agent1->device_id  = 0;
    agent1->name       = "gfx90a";
    agent1->model_name = "AMD Instinct MI200";

    auto agent2        = std::make_shared<rocprofsys::agent>();
    agent2->type       = rocprofsys::agent_type::CPU;
    agent2->device_id  = 1;
    agent2->name       = "zen3";
    agent2->model_name = "AMD EPYC";

    info::agent_t agent_info1{ agent1 };
    info::agent_t agent_info2{ agent2 };

    m_storage->add_agent_info(agent_info1);
    m_storage->add_agent_info(agent_info2);

    auto result = m_storage->get_agents();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].agent_ptr->type, rocprofsys::agent_type::GPU);
    EXPECT_EQ(result[0].agent_ptr->name, "gfx90a");
    EXPECT_EQ(result[1].agent_ptr->type, rocprofsys::agent_type::CPU);
    EXPECT_EQ(result[1].agent_ptr->name, "zen3");
}

TEST_F(metadata_parser_output_test, add_and_get_code_objects)
{
    info::code_object co1{ 1, "file:///path/to/code1.so", 0x1000, 0x500, 0, 0, 0 };
    info::code_object co2{ 2, "file:///path/to/code2.so", 0x2000, 0x600, 0, 1, 0 };

    m_storage->add_code_object(co1);
    m_storage->add_code_object(co2);

    auto result = m_storage->get_code_object_list();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].code_object_id, 1);
    EXPECT_EQ(result[0].uri, "file:///path/to/code1.so");
    EXPECT_EQ(result[1].code_object_id, 2);
    EXPECT_EQ(result[1].uri, "file:///path/to/code2.so");
}

TEST_F(metadata_parser_output_test, add_and_get_kernel_symbols)
{
    info::kernel_symbol ks1{ 100, 1, "kernel_a", 0x3000, 256, 8, 1024, 512, 32, 64, 0 };
    info::kernel_symbol ks2{
        200, 2, "kernel_b", 0x4000, 512, 16, 2048, 1024, 64, 128, 16
    };

    m_storage->add_kernel_symbol(ks1);
    m_storage->add_kernel_symbol(ks2);

    auto result = m_storage->get_kernel_symbol_list();
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].kernel_id, 100);
    EXPECT_EQ(result[0].kernel_name, "kernel_a");
    EXPECT_EQ(result[1].kernel_id, 200);
    EXPECT_EQ(result[1].kernel_name, "kernel_b");
}

TEST_F(metadata_parser_output_test, get_code_object_by_id_found)
{
    info::code_object co1{ 1, "file:///path/to/code1.so", 0x1000, 0x500, 0, 0, 0 };
    info::code_object co2{ 2, "file:///path/to/code2.so", 0x2000, 0x600, 0, 1, 0 };

    m_storage->add_code_object(co1);
    m_storage->add_code_object(co2);

    auto result = m_storage->get_code_object(2);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->code_object_id, 2);
    EXPECT_EQ(result->uri, "file:///path/to/code2.so");
}

TEST_F(metadata_parser_output_test, get_code_object_by_id_not_found)
{
    info::code_object co1{ 1, "file:///path/to/code1.so", 0x1000, 0x500, 0, 0, 0 };
    m_storage->add_code_object(co1);

    auto result = m_storage->get_code_object(999);
    EXPECT_FALSE(result.has_value());
}

TEST_F(metadata_parser_output_test, get_kernel_symbol_by_id_found)
{
    info::kernel_symbol ks1{ 100, 1, "kernel_a", 0x3000, 256, 8, 1024, 512, 32, 64, 0 };
    info::kernel_symbol ks2{
        200, 2, "kernel_b", 0x4000, 512, 16, 2048, 1024, 64, 128, 16
    };

    m_storage->add_kernel_symbol(ks1);
    m_storage->add_kernel_symbol(ks2);

    auto result = m_storage->get_kernel_symbol(100);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->kernel_id, 100);
    EXPECT_EQ(result->kernel_name, "kernel_a");
}

TEST_F(metadata_parser_output_test, get_kernel_symbol_by_id_not_found)
{
    info::kernel_symbol ks1{ 100, 1, "kernel_a", 0x3000, 256, 8, 1024, 512, 32, 64, 0 };
    m_storage->add_kernel_symbol(ks1);

    auto result = m_storage->get_kernel_symbol(999);
    EXPECT_FALSE(result.has_value());
}

TEST_F(metadata_parser_output_test, empty_lists_initially)
{
    EXPECT_TRUE(m_storage->get_pmc_info_list().empty());
    EXPECT_TRUE(m_storage->get_thread_info_list().empty());
    EXPECT_TRUE(m_storage->get_track_info_list().empty());
    EXPECT_TRUE(m_storage->get_queue_list().empty());
    EXPECT_TRUE(m_storage->get_stream_list().empty());
    EXPECT_TRUE(m_storage->get_string_list().empty());
    EXPECT_TRUE(m_storage->get_code_object_list().empty());
    EXPECT_TRUE(m_storage->get_kernel_symbol_list().empty());
    EXPECT_TRUE(m_storage->get_agents().empty());
}

class metadata_parser_handler_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_storage = std::make_shared<metadata_parser_output_t>();
        m_handler = std::make_unique<metadata_parser_handler_t>(m_storage, m_agents);
    }

    std::shared_ptr<metadata_parser_output_t>       m_storage;
    std::vector<std::shared_ptr<rocprofsys::agent>> m_agents;
    std::unique_ptr<metadata_parser_handler_t>      m_handler;
};

TEST_F(metadata_parser_handler_test, process_metadata)
{
    info::process proc{ 1234, 5678, "/usr/bin/app" };
    m_handler->execute_sample_processing(info::metadata_identifier_t::process, proc);

    auto result = m_storage->get_process_info();
    EXPECT_EQ(result.pid, 1234);
    EXPECT_EQ(result.ppid, 5678);
    EXPECT_EQ(result.command, "/usr/bin/app");
}

TEST_F(metadata_parser_handler_test, process_start_time_metadata)
{
    info::process_start_time start_time{ 1000000000 };
    m_handler->execute_sample_processing(info::metadata_identifier_t::process_start_time,
                                         start_time);

    auto result = m_storage->get_process_start_time();
    EXPECT_EQ(result.start, 1000000000);
}

TEST_F(metadata_parser_handler_test, process_end_time_metadata)
{
    info::process_end_time end_time{ 2000000000 };
    m_handler->execute_sample_processing(info::metadata_identifier_t::process_end_time,
                                         end_time);

    auto result = m_storage->get_process_end_time();
    EXPECT_EQ(result.end, 2000000000);
}

TEST_F(metadata_parser_handler_test, pmc_metadata)
{
    info::pmc pmc{ rocprofsys::agent_type::GPU,
                   0,
                   "gfx90a",
                   100,
                   0,
                   "counter1",
                   "C1",
                   "desc1",
                   "ld",
                   "comp1",
                   "cycles",
                   "u64",
                   "blk1",
                   "",
                   0,
                   0 };
    m_handler->execute_sample_processing(info::metadata_identifier_t::pmc, pmc);

    auto result = m_storage->get_pmc_info_list();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].name, "counter1");
}

TEST_F(metadata_parser_handler_test, thread_metadata)
{
    info::thread thread{ 100, 1234, 1001, 0, 100, "{}" };
    m_handler->execute_sample_processing(info::metadata_identifier_t::thread, thread);

    auto result = m_storage->get_thread_info_list();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].thread_id, 1001);
}

TEST_F(metadata_parser_handler_test, track_metadata)
{
    info::track track{ "GPU Track", std::make_optional<size_t>(1001), "{}" };
    m_handler->execute_sample_processing(info::metadata_identifier_t::track, track);

    auto result = m_storage->get_track_info_list();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].track_name, "GPU Track");
}

TEST_F(metadata_parser_handler_test, queue_metadata)
{
    info::queue queue{ 0xABCD1234 };
    m_handler->execute_sample_processing(info::metadata_identifier_t::queue, queue);

    auto result = m_storage->get_queue_list();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].handle, 0xABCD1234);
}

TEST_F(metadata_parser_handler_test, stream_metadata)
{
    info::stream stream{ 0x1111 };
    m_handler->execute_sample_processing(info::metadata_identifier_t::stream, stream);

    auto result = m_storage->get_stream_list();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].handle, 0x1111);
}

TEST_F(metadata_parser_handler_test, string_metadata)
{
    std::string        str = "test_string";
    info::string_entry entry{ str };
    m_handler->execute_sample_processing(info::metadata_identifier_t::string, entry);

    auto result = m_storage->get_string_list();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "test_string");
}

TEST_F(metadata_parser_handler_test, code_object_metadata)
{
    info::code_object co{ 1, "file:///path/to/code.so", 0x1000, 0x500, 0, 0, 0 };
    m_handler->execute_sample_processing(info::metadata_identifier_t::code_object, co);

    auto result = m_storage->get_code_object_list();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].code_object_id, 1);
    EXPECT_EQ(result[0].uri, "file:///path/to/code.so");
}

TEST_F(metadata_parser_handler_test, kernel_symbol_metadata)
{
    info::kernel_symbol ks{ 100, 1, "kernel_a", 0x3000, 256, 8, 1024, 512, 32, 64, 0 };
    m_handler->execute_sample_processing(info::metadata_identifier_t::kernel_symbol, ks);

    auto result = m_storage->get_kernel_symbol_list();
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].kernel_id, 100);
    EXPECT_EQ(result[0].kernel_name, "kernel_a");
}

TEST_F(metadata_parser_handler_test, agent_metadata)
{
    auto agent_ptr       = std::make_shared<rocprofsys::agent>();
    agent_ptr->type      = rocprofsys::agent_type::GPU;
    agent_ptr->device_id = 0;
    agent_ptr->name      = "gfx90a";

    info::agent_t agent_info{ agent_ptr };
    m_handler->execute_sample_processing(info::metadata_identifier_t::agent, agent_info);

    ASSERT_EQ(m_agents.size(), 1);
    EXPECT_EQ(m_agents[0]->type, rocprofsys::agent_type::GPU);
    EXPECT_EQ(m_agents[0]->name, "gfx90a");
}

TEST_F(metadata_parser_handler_test, unsupported_metadata_type_throws)
{
    info::process proc{ 1234, 5678, "/usr/bin/app" };
    EXPECT_THROW(m_handler->execute_sample_processing(
                     info::metadata_identifier_t::fragmented_space, proc),
                 std::runtime_error);
}

TEST_F(metadata_parser_handler_test, multiple_metadata_types)
{
    info::process            proc{ 1234, 5678, "/usr/bin/app" };
    info::process_start_time start_time{ 1000000000 };
    info::thread             thread{ 100, 1234, 1001, 0, 100, "{}" };
    info::queue              queue{ 0xABCD };

    m_handler->execute_sample_processing(info::metadata_identifier_t::process, proc);
    m_handler->execute_sample_processing(info::metadata_identifier_t::process_start_time,
                                         start_time);
    m_handler->execute_sample_processing(info::metadata_identifier_t::thread, thread);
    m_handler->execute_sample_processing(info::metadata_identifier_t::queue, queue);

    EXPECT_EQ(m_storage->get_process_info().pid, 1234);
    EXPECT_EQ(m_storage->get_process_start_time().start, 1000000000);
    ASSERT_EQ(m_storage->get_thread_info_list().size(), 1);
    ASSERT_EQ(m_storage->get_queue_list().size(), 1);
}

/*
 * Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <iomanip>
#include <functional>
#include <optional>
#include <iostream>

#include <amd_smi/amdsmi.h>
#include <amd_smi/impl/amd_smi_system.h>
#include <amd_smi/impl/amd_smi_utils.h>
#include <amd_smi/impl/amd_smi_cper.h>

namespace {
struct RAII {
    RAII(std::function<void()> init, std::function<void()> finish) 
    : _finish(finish) {
        init();
    }
    ~RAII() {
        _finish();
    }
    std::function<void()> _finish;
};

int main(int argc, char *argv[]) {

    if(argc != 2 || !argv[1]) {
        std::cout << "Missing path to a CPER file\n";
        return -1;
    }
    std::vector<uint8_t> cper_data(1024*8);
    uint64_t buf_size = cper_data.size();
    std::vector<amdsmi_cper_hdr_t> amdsmi_cper_hdrs(100);
    uint64_t entry_count = amdsmi_cper_hdrs.size();
    uint64_t cursor = 0;
    uint64_t product_serial = 0;
    amdsmi_status_t status = amdsmi_get_gpu_cper_entries_by_path(
        argv[1], //const char *amdgpu_ring_cper_file, 
        AMDSMI_CPER_SEV_FATAL|AMDSMI_CPER_SEV_NON_FATAL_CORRECTED|AMDSMI_CPER_SEV_NON_FATAL_UNCORRECTED,uint32_t severity_mask,
        cper_data.data(), //char *cper_data, 
        &buf_size, //uint64_t *buf_size, 
        amdsmi_cper_hdrs.data(), //amdsmi_cper_hdr_t **cper_hdrs,
        &entry_count, //uint64_t *entry_count, 
        &cursor, 
        product_serial);


    return 0;
}
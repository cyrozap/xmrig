/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "base/tools/String.h"


#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <thread>

#include <sys/auxv.h>
#include <asm/cputable.h>

#include "backend/cpu/platform/BasicCpuInfo.h"
#include "3rdparty/rapidjson/document.h"


namespace xmrig {

extern String cpu_name_power();

} // namespace xmrig


xmrig::BasicCpuInfo::BasicCpuInfo() :
    m_threads(std::thread::hardware_concurrency())
{
    m_units.resize(m_threads);
    for (int32_t i = 0; i < static_cast<int32_t>(m_threads); ++i) {
        m_units[i] = i;
    }

    String name = cpu_name_power();
    if (!name.isNull()) {
        strncpy(m_brand, name, sizeof(m_brand) - 1);
        m_brand[sizeof(m_brand) - 1] = '\0';
    } else {
        unsigned long platform_val = getauxval(AT_PLATFORM);
        if (platform_val != 0) {
            const char *platform = (const char *)platform_val;
            if (strncmp(platform, "power", 5) == 0) {
                int version = std::atoi(platform + 5);
                if (version > 0 && version < 10) {
                    snprintf(m_brand, sizeof(m_brand), "POWER%s", platform + 5);
                } else if (version >= 10) {
                    snprintf(m_brand, sizeof(m_brand), "Power%s", platform + 5);
                } else {
                    strncpy(m_brand, platform, sizeof(m_brand) - 1);
                    m_brand[sizeof(m_brand) - 1] = '\0';
                }
            } else {
                strncpy(m_brand, platform, sizeof(m_brand) - 1);
                m_brand[sizeof(m_brand) - 1] = '\0';
            }
        } else {
            memcpy(m_brand, "POWER", 6);
        }
    }

    unsigned long hwcaps2 = getauxval(AT_HWCAP2);

    m_flags.set(FLAG_AES, (hwcaps2 & PPC_FEATURE2_VEC_CRYPTO) != 0);
    m_flags.set(FLAG_POWER_V3P0, (hwcaps2 & PPC_FEATURE2_ARCH_3_00) != 0);

    m_flags.set(FLAG_PDPE1GB, std::ifstream("/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages").good());
}


const char *xmrig::BasicCpuInfo::backend() const
{
    return "basic/1";
}


xmrig::CpuThreads xmrig::BasicCpuInfo::threads(const Algorithm &algorithm, uint32_t) const
{
    // Divide by four threads per core (SMT4 assumed)
    size_t physical_cores = std::max<size_t>(threads() / 4, 1);

#   ifdef XMRIG_ALGO_GHOSTRIDER
    if (algorithm.family() == Algorithm::GHOSTRIDER) {
        return CpuThreads(physical_cores, 8);
    }
#   endif

    return CpuThreads(physical_cores, 1);
}


rapidjson::Value xmrig::BasicCpuInfo::toJSON(rapidjson::Document &doc) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    Value out(kObjectType);

    out.AddMember("brand",      StringRef(brand()), allocator);
    out.AddMember("aes",        hasAES(), allocator);
    out.AddMember("avx2",       false, allocator);
    out.AddMember("x64",        is64bit(), allocator); // DEPRECATED will be removed in the next major release.
    out.AddMember("64_bit",     is64bit(), allocator);
    out.AddMember("l2",         static_cast<uint64_t>(L2()), allocator);
    out.AddMember("l3",         static_cast<uint64_t>(L3()), allocator);
    out.AddMember("cores",      static_cast<uint64_t>(cores()), allocator);
    out.AddMember("threads",    static_cast<uint64_t>(threads()), allocator);
    out.AddMember("packages",   static_cast<uint64_t>(packages()), allocator);
    out.AddMember("nodes",      static_cast<uint64_t>(nodes()), allocator);
    out.AddMember("backend",    StringRef(backend()), allocator);
    out.AddMember("msr",        "none", allocator);
    out.AddMember("assembly",   "none", allocator);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    out.AddMember("arch",       "ppc64", allocator);
#else
    out.AddMember("arch",       "ppc64le", allocator);
#endif

    Value flags(kArrayType);

    unsigned long hwcaps = getauxval(AT_HWCAP);
    unsigned long hwcaps2 = getauxval(AT_HWCAP2);

    if (hwcaps & PPC_FEATURE_HAS_ALTIVEC) {
        flags.PushBack("altivec", allocator);
    }
    if (hwcaps & PPC_FEATURE_HAS_VSX) {
        flags.PushBack("vsx", allocator);
    }
    if (hwcaps2 & PPC_FEATURE2_VEC_CRYPTO) {
        flags.PushBack("vcrypto", allocator);
    }
    if (hwcaps & PPC_FEATURE_ARCH_2_06) {
        flags.PushBack("arch_2_06", allocator);
    }
    if (hwcaps2 & PPC_FEATURE2_ARCH_2_07) {
        flags.PushBack("arch_2_07", allocator);
    }
    if (hwcaps2 & PPC_FEATURE2_ARCH_3_00) {
        flags.PushBack("arch_3_00", allocator);
    }
    if (hwcaps2 & PPC_FEATURE2_ARCH_3_1) {
        flags.PushBack("arch_3_1", allocator);
    }

    out.AddMember("flags", flags, allocator);

    return out;
}

/* XMRig
 * Copyright (c) 2018      Riku Voipio <riku.voipio@iki.fi>
 * Copyright (c) 2018-2023 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2023 XMRig       <support@xmrig.com>
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
#include "3rdparty/fmt/core.h"


#include <cstdio>
#include <cctype>


namespace xmrig {


struct lscpu_desc
{
    String model;
    String revision;

    inline bool isReady() const { return !model.isNull() && !revision.isNull(); }
};


static bool lookup(char *line, const char *pattern, String &value)
{
    if (!*line || !value.isNull()) {
        return false;
    }

    char *p;
    int len = strlen(pattern);

    if (strncmp(line, pattern, len) != 0) {
        return false;
    }

    for (p = line + len; isspace(*p); p++);

    if (*p != ':') {
        return false;
    }

    for (++p; isspace(*p); p++);

    if (!*p) {
        return false;
    }

    const char *v = p;

    len = strlen(line) - 1;
    for (p = line + len; isspace(*(p-1)); p--);
    *p = '\0';

    value = v;

    return true;
}


static bool read_basicinfo(lscpu_desc *desc)
{
    auto fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        return false;
    }

    char buf[BUFSIZ];
    while (fgets(buf, sizeof(buf), fp) != nullptr) {
        if (!lookup(buf, "cpu", desc->model)) {
            lookup(buf, "revision", desc->revision);
        }

        if (desc->isReady()) {
            break;
        }
    }

    fclose(fp);

    return desc->isReady();
}


String cpu_name_power()
{
    lscpu_desc desc;
    if (read_basicinfo(&desc)) {
        return fmt::format("{}, {}", desc.model, desc.revision).c_str();
    }

    return {};
}


} // namespace xmrig

/*
 *   Copyright (c) 2026 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "PanIdAssignmentFile.h"

#include <lib/support/logging/CHIPLogging.h>

#include <cctype>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

namespace chip {
namespace tool {

namespace {

bool ParseNetworkKey(const std::string & hex, uint8_t * out, size_t outLen)
{
    if (hex.size() != outLen * 2)
    {
        return false;
    }
    for (size_t i = 0; i < outLen; i++)
    {
        unsigned int byte = 0;
        if (sscanf(hex.c_str() + i * 2, "%02x", &byte) != 1)
        {
            return false;
        }
        out[i] = static_cast<uint8_t>(byte);
    }
    return true;
}

void Trim(std::string & s)
{
    while (!s.empty() && isspace(static_cast<unsigned char>(s.front())))
    {
        s.erase(s.begin());
    }
    while (!s.empty() && isspace(static_cast<unsigned char>(s.back())))
    {
        s.pop_back();
    }
}

} // namespace

CHIP_ERROR PanIdAssignmentFile::Load(const char * filePath)
{
    mFilePath    = filePath;
    mCounterPath = mFilePath + ".counter";
    mEntries.clear();

    std::ifstream file(mFilePath);
    if (!file.is_open())
    {
        ChipLogError(chipTool, "Cannot open PAN ID assignment file: %s", filePath);
        return CHIP_ERROR_OPEN_FAILED;
    }

    std::string line;
    uint32_t lineNum = 0;

    while (std::getline(file, line))
    {
        lineNum++;
        Trim(line);

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        auto comma = line.find(',');
        if (comma == std::string::npos)
        {
            ChipLogError(chipTool, "Assignment file line %u: missing comma, skipping", lineNum);
            continue;
        }

        std::string panIdStr      = line.substr(0, comma);
        std::string networkKeyStr = line.substr(comma + 1);
        Trim(panIdStr);
        Trim(networkKeyStr);

        PanIdAssignmentEntry entry;

        try
        {
            entry.panId = static_cast<uint16_t>(std::stoul(panIdStr, nullptr, 16));
        }
        catch (const std::exception &)
        {
            ChipLogError(chipTool, "Assignment file line %u: invalid PAN ID '%s', skipping", lineNum, panIdStr.c_str());
            continue;
        }

        if (!ParseNetworkKey(networkKeyStr, entry.networkKey, kPanIdAssignmentNetworkKeySize))
        {
            ChipLogError(chipTool, "Assignment file line %u: invalid network key '%s' (expected 32 hex chars), skipping",
                         lineNum, networkKeyStr.c_str());
            continue;
        }

        mEntries.push_back(entry);
    }

    if (mEntries.empty())
    {
        ChipLogError(chipTool, "No valid entries found in assignment file: %s", filePath);
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    ReturnErrorOnFailure(ReadCounter());

    ChipLogProgress(chipTool, "Loaded %zu PAN ID assignments from '%s', counter=%u", mEntries.size(), filePath, mCounter);
    return CHIP_NO_ERROR;
}

CHIP_ERROR PanIdAssignmentFile::ReadCounter()
{
    std::ifstream file(mCounterPath);
    if (!file.is_open())
    {
        mCounter = 0;
        return CHIP_NO_ERROR;
    }
    if (!(file >> mCounter))
    {
        mCounter = 0;
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR PanIdAssignmentFile::WriteCounter()
{
    std::ofstream file(mCounterPath, std::ios::trunc);
    if (!file.is_open())
    {
        ChipLogError(chipTool, "Cannot write counter file: %s", mCounterPath.c_str());
        return CHIP_ERROR_WRITE_FAILED;
    }
    file << mCounter;
    return CHIP_NO_ERROR;
}

CHIP_ERROR PanIdAssignmentFile::GetNext(PanIdAssignmentEntry & entry)
{
    VerifyOrReturnError(!mEntries.empty(), CHIP_ERROR_INVALID_ARGUMENT);

    uint32_t index = mCounter % static_cast<uint32_t>(mEntries.size());
    entry          = mEntries[index];

    mCounter++;
    ReturnErrorOnFailure(WriteCounter());

    ChipLogProgress(chipTool, "PAN ID assignment: index=%u PAN=0x%04x", index, entry.panId);
    return CHIP_NO_ERROR;
}

} // namespace tool
} // namespace chip

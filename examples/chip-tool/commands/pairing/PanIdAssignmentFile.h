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

#pragma once

#include <lib/core/CHIPError.h>

#include <stdint.h>
#include <string>
#include <vector>

namespace chip {
namespace tool {

static constexpr size_t kPanIdAssignmentNetworkKeySize = 16;

struct PanIdAssignmentEntry
{
    uint16_t panId;
    uint8_t  networkKey[kPanIdAssignmentNetworkKeySize];
};

/**
 * Loads a CSV assignment file of the form:
 *
 *   # comment lines are ignored
 *   0xABCD,00112233445566778899aabbccddeeff
 *   0x1234,aabbccddeeff00112233445566778899
 *
 * A counter is persisted in <filePath>.counter beside the file.
 * Each call to GetNext() returns the entry at the current counter index
 * (wrapping around the pool size) and increments the counter.
 *
 * Delete the .counter file to reset the counter to 0.
 */
class PanIdAssignmentFile
{
public:
    CHIP_ERROR Load(const char * filePath);
    CHIP_ERROR GetNext(PanIdAssignmentEntry & entry);

    size_t Size() const { return mEntries.size(); }
    uint32_t Counter() const { return mCounter; }

private:
    CHIP_ERROR ReadCounter();
    CHIP_ERROR WriteCounter();

    std::string mFilePath;
    std::string mCounterPath;
    std::vector<PanIdAssignmentEntry> mEntries;
    uint32_t mCounter = 0;
};

} // namespace tool
} // namespace chip

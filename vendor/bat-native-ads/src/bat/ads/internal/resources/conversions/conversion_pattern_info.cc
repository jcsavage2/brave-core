/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/conversions/conversion_pattern_info.h"

namespace ads {

ConversionPatternInfo::ConversionPatternInfo() = default;

ConversionPatternInfo::ConversionPatternInfo(
    const ConversionPatternInfo& info) = default;

ConversionPatternInfo::~ConversionPatternInfo() = default;

bool ConversionPatternInfo::operator==(const ConversionPatternInfo& rhs) const {
  return conversion_url == rhs.conversion_url && type == rhs.type &&
         script == rhs.script;
}

bool ConversionPatternInfo::operator!=(const ConversionPatternInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads

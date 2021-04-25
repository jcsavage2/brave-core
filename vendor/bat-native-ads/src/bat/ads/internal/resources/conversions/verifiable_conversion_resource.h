/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_VERIFIABLE_CONVERSION_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_VERIFIABLE_CONVERSION_RESOURCE_H_

#include <string>

#include "bat/ads/internal/resources/conversions/conversion_pattern_info.h"
#include "bat/ads/internal/resources/resource.h"

namespace ads {
namespace resource {

class VerifiableConversion : public Resource<ConversionPatternMap> {
 public:
  VerifiableConversion();
  ~VerifiableConversion() override;

  VerifiableConversion(const VerifiableConversion&) = delete;
  VerifiableConversion& operator=(const VerifiableConversion&) = delete;

  bool IsInitialized() const override;

  void Load();

  ConversionPatternMap get() const override;

 private:
  bool is_initialized_ = false;

  ConversionPatternMap conversion_patterns_;

  bool FromJson(const std::string& json);
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_CONVERSIONS_VERIFIABLE_CONVERSION_RESOURCE_H_

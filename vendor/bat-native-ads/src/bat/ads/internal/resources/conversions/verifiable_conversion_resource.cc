/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/conversions/verifiable_conversion_resource.h"

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/result.h"

namespace ads {
namespace resource {

namespace {
const char kResourceId[] = "nnqccijfhvzwyrxpxwjrpmynaiazctqb";
const char kVersionId = 1;
}  // namespace

VerifiableConversion::VerifiableConversion() = default;

VerifiableConversion::~VerifiableConversion() = default;

bool VerifiableConversion::IsInitialized() const {
  return is_initialized_;
}

void VerifiableConversion::Load() {
  AdsClientHelper::Get()->LoadAdsResource(
      kResourceId, kVersionId,
      [=](const Result result, const std::string& json) {
        if (result != SUCCESS) {
          BLOG(1, "Failed to load resource " << kResourceId);
          is_initialized_ = false;
          return;
        }

        BLOG(1, "Successfully loaded resource " << kResourceId);

        if (!FromJson(json)) {
          BLOG(1, "Failed to initialize resource " << kResourceId);
          is_initialized_ = false;
          return;
        }

        is_initialized_ = true;

        BLOG(1, "Successfully initialized resource " << kResourceId);
      });
}

ConversionPatternMap VerifiableConversion::get() const {
  return conversion_patterns_;
}

///////////////////////////////////////////////////////////////////////////////

bool VerifiableConversion::FromJson(const std::string& json) {
  ConversionPatternMap conversion_patterns;

  base::Optional<base::Value> root = base::JSONReader::Read(json);
  if (!root) {
    BLOG(1, "Failed to load from JSON, root missing");
    return false;
  }

  if (base::Optional<int> version = root->FindIntPath("version")) {
    if (kVersionId != *version) {
      BLOG(1, "Failed to load from JSON, version missing");
      return false;
    }
  }

  base::Value* conversion_patterns_value =
      root->FindDictPath("conversion_patterns");
  if (!conversion_patterns_value) {
    BLOG(1, "Failed to load from JSON, conversion patterns missing");
    return false;
  }

  if (!conversion_patterns_value->is_dict()) {
    BLOG(1, "Failed to load from JSON, conversion patterns not of type dict");
    return false;
  }

  base::DictionaryValue* dict;
  if (!conversion_patterns_value->GetAsDictionary(&dict)) {
    BLOG(1, "Failed to load from JSON, get conversion patterns as dict");
    return false;
  }

  for (base::DictionaryValue::Iterator iter(*dict); !iter.IsAtEnd();
       iter.Advance()) {
    if (!iter.value().is_dict()) {
      BLOG(1, "Failed to load from JSON, conversion pattern not of type dict")
      return false;
    }

    const std::string* type = iter.value().FindStringKey("type");
    if (type->empty()) {
      BLOG(1, "Failed to load from JSON, conversion pattern type missing");
      return false;
    }

    const std::string* script = iter.value().FindStringKey("script");
    if (script->empty()) {
      BLOG(1, "Failed to load from JSON, conversion pattern script missing");
      return false;
    }

    ConversionPatternInfo info;
    info.conversion_url = iter.key();
    info.type = *type;
    info.script = *script;
    conversion_patterns.insert({info.conversion_url, info});
  }

  conversion_patterns_ = conversion_patterns;

  BLOG(1, "Parsed verifiable conversion resource version " << kVersionId);

  return true;
}

}  // namespace resource
}  // namespace ads

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/conversions/verifiable_conversion_resource.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace resource {

class BatAdsVerifiableConversionResourceTest : public UnitTestBase {
 protected:
  BatAdsVerifiableConversionResourceTest() = default;

  ~BatAdsVerifiableConversionResourceTest() override = default;
};

TEST_F(BatAdsVerifiableConversionResourceTest, Load) {
  // Arrange
  resource::VerifiableConversion resource;

  // Act
  resource.Load();

  // Assert
  const bool is_initialized = resource.IsInitialized();
  EXPECT_TRUE(is_initialized);
}

TEST_F(BatAdsVerifiableConversionResourceTest, Get) {
  // Arrange
  resource::VerifiableConversion resource;
  resource.Load();

  // Act
  ConversionPatternMap conversion_patterns = resource.get();

  // Assert
  EXPECT_EQ(3u, conversion_patterns.size());
}

}  // namespace resource
}  // namespace ads

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { createAction } from 'redux-act'
import { ViewType, AssetDetail, ConversionQuote } from './ftx_state'

type InitializedPayload = {
  isConnected: boolean
  balances: chrome.ftx.Balances
  marketData: chrome.ftx.TokenPriceData[]
}

export const initialize = createAction('initialize')

export const initialized = createAction<InitializedPayload>('initialized')

export const openView = createAction<ViewType>('openView')

export const startConnectToFtx = createAction('startConnectToFtx')

export type ShowAssetDetailPayload = {
  symbol: string
}
export const showAssetDetail = createAction<ShowAssetDetailPayload>('showAssetDetail')

export const hideAssetDetail = createAction('hideAssetDetail')

export const assetChartDataUpdated = createAction<AssetDetail>('assetChartDataUpdated')

export const toggleOptInMarkets = createAction<boolean>('toggleOptInMarkets')

export const interacted = createAction('interacted')

export type PreviewConversionPayload = {
  from: string
  to: string
  quantity: number
}
export const previewConversion = createAction<PreviewConversionPayload>('previewConversion')

export type ConversionQuoteAvailablePayload = ConversionQuote
export const conversionQuoteAvailable = createAction<ConversionQuoteAvailablePayload>('conversionQuoteAvailable')

export const cancelConversion = createAction('cancelConversion')

export const submitConversion = createAction('submitConversion')

export const closeConversion = createAction('closeConversion')

export const conversionWasSuccessful = createAction('conversionWasSuccessful')
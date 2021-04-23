// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { TradingDropdown } from '../../shared'
import * as S from '../../shared/styles'
import * as FTXActions from '../ftx_actions'
import { FTXState } from '../ftx_state'

type Props = {
  ftx: FTXState
  actions: typeof FTXActions
}

type Choices = {
  from: string
  to: string
  quantity: number
}

const doNothing = () => { console.log('Action doNothing was fired') }

function ConversionInProgress (props: Props) {
  const data = props.ftx.conversionInProgress
  // Validate
  if (!data) {
    return null
  }
  const isFetchingQuote = !data.quote
  // 1 minute countdown
  const [timeLeft, setTimeLeft] = React.useState(60)
  // This should run every time timeLeft changes, which is every second
  React.useEffect(() => {
    // Do nothing if the user clicked submit
    if (data.isSubmitting || isFetchingQuote) {
      return
    }
    setTimeout(() => {
      // Cancel if we're at 0
      if (timeLeft === 0) {
        props.actions.cancelConversion()
        return
      }
      // Count down
      setTimeLeft(timeLeft - 1)
    }, 1000)
  }, [timeLeft, setTimeLeft, data.isSubmitting, isFetchingQuote])
  return (
    <S.BasicBox $mt={10} $mb={10}>
      <S.Text>
        Confirm Conversion
      </S.Text>
      <S.BasicBox $mb={20}>
        <S.BasicBox isFlex={true}>
          <S.LightText small={true}>Selling</S.LightText>
          <S.Text small={true}>{data.quote ? data.quote.cost : data.quantity} {data.from}</S.Text>
        </S.BasicBox>
        {data.quote &&
        <>
        <S.BasicBox isFlex={true}>
          <S.LightText small={true}>Price</S.LightText>
          <S.Text small={true}>{data.quote.price} {data.to}</S.Text>
        </S.BasicBox>
        <S.BasicBox isFlex={true}>
          <S.LightText small={true}>Proceeds</S.LightText>
          <S.Text small={true}>{data.quote.proceeds} {data.to}</S.Text>
        </S.BasicBox>
        </>
        }
        {isFetchingQuote &&
          <S.Text small={true} center={true}>Loading...</S.Text>
        }
      </S.BasicBox>
      {!isFetchingQuote &&
      <S.ActionButton disabled={data.isSubmitting} onClick={!data.isSubmitting ? props.actions.submitConversion : doNothing}>
        {data.isSubmitting
          ? <>Submitting...</>
          : <>Confirm ({timeLeft}s)</>
        }
      </S.ActionButton>
      }
    </S.BasicBox>
  )
}

function ConversionSuccessful (props: Props) {
  return (
    <S.BasicBox isFlex={true} $mt={25} $mb={25}>
      <S.Text>Conversion Successful!</S.Text>
      <S.ActionButton onClick={props.actions.cancelConversion}>
        Close
      </S.ActionButton>
    </S.BasicBox>
  )
}

export default function Convert (props: Props) {
  const [choices, setChoices] = React.useState<Choices>({ from: '', to: '', quantity: 0 })

  const onChange = React.useCallback((from: string, to: string, quantity: number) => {
    setChoices({ from, to, quantity })
  }, [setChoices])

  const availableAmount = React.useMemo(() => {
    const amount = props.ftx.balances[choices.from] || 0
    return Number(amount.toFixed(4))
  }, [choices.from])

  const previewAction = React.useCallback(() => {
    props.actions.previewConversion({
      from: choices.from,
      to: choices.to,
      quantity: choices.quantity
    })
  }, [choices, props.actions.previewConversion])

  // Handle user has no balance
  const hasBalances = !!props.ftx.balanceTotal
  if (!hasBalances) {
    return (
      <>
        <S.BasicBox isFlex={true} $mt={25} $mb={25}>
          <S.Text>You need a balance in at least one currency in order to perform a conversion.</S.Text>
        </S.BasicBox>
      </>
    )
  }

  // Handle conversion in progress
  const hasConversionInProgress = !!props.ftx.conversionInProgress
  if (hasConversionInProgress) {
    // Handle Success
    if (props.ftx.conversionInProgress?.complete) {
      return <ConversionSuccessful ftx={props.ftx} actions={props.actions} />
    }
    return <ConversionInProgress ftx={props.ftx} actions={props.actions} />
  }

  return (
    <>
      <S.BasicBox isFlex={true} $mt={25}>
        <S.Text>
          Convert
        </S.Text>
        <S.Text small={true}>
          {`Available ${availableAmount} ${choices.from}`}
        </S.Text>
      </S.BasicBox>
      <TradingDropdown
        fromAssets={Object.keys(props.ftx.balances)}
        toAssets={props.ftx.currencyNames}
        onChange={onChange}
      />
      <S.ActionsWrapper>
        <S.ActionButton onClick={previewAction}>
          Preview Conversion
        </S.ActionButton>
      </S.ActionsWrapper>
    </>
  )
}

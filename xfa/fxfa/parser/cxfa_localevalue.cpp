// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_localevalue.h"

#include <vector>

#include "core/fxcrt/fx_extension.h"
#include "third_party/base/ptr_util.h"
#include "third_party/base/stl_util.h"
#include "xfa/fgas/crt/cfgas_formatstring.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_localemgr.h"
#include "xfa/fxfa/parser/xfa_utils.h"

namespace {

FX_LOCALECATEGORY ValueCategory(FX_LOCALECATEGORY eCategory,
                                uint32_t dwValueType) {
  if (eCategory != FX_LOCALECATEGORY_Unknown)
    return eCategory;

  switch (dwValueType) {
    case XFA_VT_BOOLEAN:
    case XFA_VT_INTEGER:
    case XFA_VT_DECIMAL:
    case XFA_VT_FLOAT:
      return FX_LOCALECATEGORY_Num;
    case XFA_VT_TEXT:
      return FX_LOCALECATEGORY_Text;
    case XFA_VT_DATE:
      return FX_LOCALECATEGORY_Date;
    case XFA_VT_TIME:
      return FX_LOCALECATEGORY_Time;
    case XFA_VT_DATETIME:
      return FX_LOCALECATEGORY_DateTime;
  }
  return FX_LOCALECATEGORY_Unknown;
}

bool ValueSplitDateTime(const WideString& wsDateTime,
                        WideString& wsDate,
                        WideString& wsTime) {
  wsDate = L"";
  wsTime = L"";
  if (wsDateTime.IsEmpty())
    return false;

  auto nSplitIndex = wsDateTime.Find('T');
  if (!nSplitIndex.has_value())
    nSplitIndex = wsDateTime.Find(' ');
  if (!nSplitIndex.has_value())
    return false;

  wsDate = wsDateTime.Left(nSplitIndex.value());
  wsTime = wsDateTime.Right(wsDateTime.GetLength() - nSplitIndex.value() - 1);
  return true;
}

}  // namespace

CXFA_LocaleValue::CXFA_LocaleValue()
    : m_pLocaleMgr(nullptr), m_dwType(XFA_VT_NULL), m_bValid(true) {}

CXFA_LocaleValue::CXFA_LocaleValue(const CXFA_LocaleValue& value)
    : m_pLocaleMgr(value.m_pLocaleMgr),
      m_wsValue(value.m_wsValue),
      m_dwType(value.m_dwType),
      m_bValid(value.m_bValid) {}

CXFA_LocaleValue::CXFA_LocaleValue(uint32_t dwType, CXFA_LocaleMgr* pLocaleMgr)
    : m_pLocaleMgr(pLocaleMgr),
      m_dwType(dwType),
      m_bValid(m_dwType != XFA_VT_NULL) {}

CXFA_LocaleValue::CXFA_LocaleValue(uint32_t dwType,
                                   const WideString& wsValue,
                                   CXFA_LocaleMgr* pLocaleMgr)
    : m_pLocaleMgr(pLocaleMgr),
      m_wsValue(wsValue),
      m_dwType(dwType),
      m_bValid(ValidateCanonicalValue(wsValue, dwType)) {}

CXFA_LocaleValue::CXFA_LocaleValue(uint32_t dwType,
                                   const WideString& wsValue,
                                   const WideString& wsFormat,
                                   IFX_Locale* pLocale,
                                   CXFA_LocaleMgr* pLocaleMgr)
    : m_pLocaleMgr(pLocaleMgr),
      m_dwType(dwType),
      m_bValid(ParsePatternValue(wsValue, wsFormat, pLocale)) {}

CXFA_LocaleValue& CXFA_LocaleValue::operator=(const CXFA_LocaleValue& value) {
  m_wsValue = value.m_wsValue;
  m_dwType = value.m_dwType;
  m_bValid = value.m_bValid;
  m_pLocaleMgr = value.m_pLocaleMgr;
  return *this;
}

CXFA_LocaleValue::~CXFA_LocaleValue() {}

bool CXFA_LocaleValue::ValidateValue(const WideString& wsValue,
                                     const WideString& wsPattern,
                                     IFX_Locale* pLocale,
                                     WideString* pMatchFormat) {
  WideString wsOutput;
  IFX_Locale* locale = m_pLocaleMgr->GetDefLocale();
  if (pLocale)
    m_pLocaleMgr->SetDefLocale(pLocale);

  auto pFormat = pdfium::MakeUnique<CFGAS_FormatString>(m_pLocaleMgr);
  std::vector<WideString> wsPatterns;
  pFormat->SplitFormatString(wsPattern, &wsPatterns);

  bool bRet = false;
  int32_t iCount = pdfium::CollectionSize<int32_t>(wsPatterns);
  int32_t i = 0;
  for (; i < iCount && !bRet; i++) {
    WideString wsFormat = wsPatterns[i];
    switch (ValueCategory(pFormat->GetCategory(wsFormat), m_dwType)) {
      case FX_LOCALECATEGORY_Null:
        bRet = pFormat->ParseNull(wsValue, wsFormat);
        if (!bRet)
          bRet = wsValue.IsEmpty();
        break;
      case FX_LOCALECATEGORY_Zero:
        bRet = pFormat->ParseZero(wsValue, wsFormat);
        if (!bRet)
          bRet = wsValue == L"0";
        break;
      case FX_LOCALECATEGORY_Num: {
        WideString fNum;
        bRet = pFormat->ParseNum(wsValue, wsFormat, &fNum);
        if (!bRet)
          bRet = pFormat->FormatNum(wsValue, wsFormat, &wsOutput);
        break;
      }
      case FX_LOCALECATEGORY_Text:
        bRet = pFormat->ParseText(wsValue, wsFormat, &wsOutput);
        wsOutput.clear();
        if (!bRet)
          bRet = pFormat->FormatText(wsValue, wsFormat, &wsOutput);
        break;
      case FX_LOCALECATEGORY_Date: {
        CFX_DateTime dt;
        bRet = ValidateCanonicalDate(wsValue, &dt);
        if (!bRet) {
          bRet = pFormat->ParseDateTime(wsValue, wsFormat, FX_DATETIMETYPE_Date,
                                        &dt);
          if (!bRet) {
            bRet = pFormat->FormatDateTime(wsValue, wsFormat,
                                           FX_DATETIMETYPE_Date, &wsOutput);
          }
        }
        break;
      }
      case FX_LOCALECATEGORY_Time: {
        CFX_DateTime dt;
        bRet = pFormat->ParseDateTime(wsValue, wsFormat, FX_DATETIMETYPE_Time,
                                      &dt);
        if (!bRet) {
          bRet = pFormat->FormatDateTime(wsValue, wsFormat,
                                         FX_DATETIMETYPE_Time, &wsOutput);
        }
        break;
      }
      case FX_LOCALECATEGORY_DateTime: {
        CFX_DateTime dt;
        bRet = pFormat->ParseDateTime(wsValue, wsFormat,
                                      FX_DATETIMETYPE_DateTime, &dt);
        if (!bRet) {
          bRet = pFormat->FormatDateTime(wsValue, wsFormat,
                                         FX_DATETIMETYPE_DateTime, &wsOutput);
        }
        break;
      }
      default:
        bRet = false;
        break;
    }
  }
  if (bRet && pMatchFormat)
    *pMatchFormat = wsPatterns[i - 1];
  if (pLocale)
    m_pLocaleMgr->SetDefLocale(locale);

  return bRet;
}

double CXFA_LocaleValue::GetDoubleNum() const {
  if (m_bValid && (m_dwType == XFA_VT_BOOLEAN || m_dwType == XFA_VT_INTEGER ||
                   m_dwType == XFA_VT_DECIMAL || m_dwType == XFA_VT_FLOAT)) {
    int64_t nIntegral = 0;
    uint32_t dwFractional = 0;
    int32_t nExponent = 0;
    int32_t cc = 0;
    bool bNegative = false;
    bool bExpSign = false;
    const wchar_t* str = m_wsValue.c_str();
    int len = m_wsValue.GetLength();
    while (FXSYS_iswspace(str[cc]) && cc < len)
      cc++;

    if (cc >= len)
      return 0;
    if (str[0] == '+') {
      cc++;
    } else if (str[0] == '-') {
      bNegative = true;
      cc++;
    }

    int32_t nIntegralLen = 0;
    while (cc < len) {
      if (str[cc] == '.' || !FXSYS_isDecimalDigit(str[cc]) ||
          nIntegralLen > 17) {
        break;
      }
      nIntegral = nIntegral * 10 + str[cc] - '0';
      cc++;
      nIntegralLen++;
    }

    nIntegral = bNegative ? -nIntegral : nIntegral;
    int32_t scale = 0;
    double fraction = 0.0;
    if (cc < len && str[cc] == '.') {
      cc++;
      while (cc < len) {
        fraction += XFA_GetFractionalScale(scale) * (str[cc] - '0');
        scale++;
        cc++;
        if (scale == XFA_GetMaxFractionalScale() ||
            !FXSYS_isDecimalDigit(str[cc])) {
          break;
        }
      }
      dwFractional = static_cast<uint32_t>(fraction * 4294967296.0);
    }
    if (cc < len && (str[cc] == 'E' || str[cc] == 'e')) {
      cc++;
      if (cc < len) {
        if (str[cc] == '+') {
          cc++;
        } else if (str[cc] == '-') {
          bExpSign = true;
          cc++;
        }
      }
      while (cc < len) {
        if (str[cc] == '.' || !FXSYS_isDecimalDigit(str[cc]))
          break;

        nExponent = nExponent * 10 + str[cc] - '0';
        cc++;
      }
      nExponent = bExpSign ? -nExponent : nExponent;
    }

    double dValue = dwFractional / 4294967296.0;
    dValue = nIntegral + (nIntegral >= 0 ? dValue : -dValue);
    if (nExponent != 0)
      dValue *= FXSYS_pow(10, static_cast<float>(nExponent));

    return dValue;
  }
  return 0;
}

CFX_DateTime CXFA_LocaleValue::GetDate() const {
  if (!m_bValid || m_dwType != XFA_VT_DATE)
    return CFX_DateTime();

  CFX_DateTime dt;
  FX_DateFromCanonical(m_wsValue, &dt);
  return dt;
}

CFX_DateTime CXFA_LocaleValue::GetTime() const {
  if (!m_bValid || m_dwType != XFA_VT_TIME)
    return CFX_DateTime();

  CFX_DateTime dt;
  FX_TimeFromCanonical(m_wsValue.AsStringView(), &dt,
                       m_pLocaleMgr->GetDefLocale());
  return dt;
}

bool CXFA_LocaleValue::SetDate(const CFX_DateTime& d) {
  m_dwType = XFA_VT_DATE;
  m_wsValue = WideString::Format(L"%04d-%02d-%02d", d.GetYear(), d.GetMonth(),
                                 d.GetDay());
  return true;
}

bool CXFA_LocaleValue::SetTime(const CFX_DateTime& t) {
  m_dwType = XFA_VT_TIME;
  m_wsValue = WideString::Format(L"%02d:%02d:%02d", t.GetHour(), t.GetMinute(),
                                 t.GetSecond());
  if (t.GetMillisecond() > 0)
    m_wsValue += WideString::Format(L"%:03d", t.GetMillisecond());
  return true;
}

bool CXFA_LocaleValue::SetDateTime(const CFX_DateTime& dt) {
  m_dwType = XFA_VT_DATETIME;
  m_wsValue = WideString::Format(L"%04d-%02d-%02dT%02d:%02d:%02d", dt.GetYear(),
                                 dt.GetMonth(), dt.GetDay(), dt.GetHour(),
                                 dt.GetMinute(), dt.GetSecond());
  if (dt.GetMillisecond() > 0)
    m_wsValue += WideString::Format(L"%:03d", dt.GetMillisecond());
  return true;
}

bool CXFA_LocaleValue::FormatPatterns(WideString& wsResult,
                                      const WideString& wsFormat,
                                      IFX_Locale* pLocale,
                                      XFA_VALUEPICTURE eValueType) const {
  auto pFormat = pdfium::MakeUnique<CFGAS_FormatString>(m_pLocaleMgr);
  std::vector<WideString> wsPatterns;
  pFormat->SplitFormatString(wsFormat, &wsPatterns);
  wsResult.clear();
  int32_t iCount = pdfium::CollectionSize<int32_t>(wsPatterns);
  for (int32_t i = 0; i < iCount; i++) {
    if (FormatSinglePattern(wsResult, wsPatterns[i], pLocale, eValueType))
      return true;
  }
  return false;
}

bool CXFA_LocaleValue::FormatSinglePattern(WideString& wsResult,
                                           const WideString& wsFormat,
                                           IFX_Locale* pLocale,
                                           XFA_VALUEPICTURE eValueType) const {
  IFX_Locale* locale = m_pLocaleMgr->GetDefLocale();
  if (pLocale)
    m_pLocaleMgr->SetDefLocale(pLocale);

  wsResult.clear();
  bool bRet = false;
  auto pFormat = pdfium::MakeUnique<CFGAS_FormatString>(m_pLocaleMgr);
  FX_LOCALECATEGORY eCategory =
      ValueCategory(pFormat->GetCategory(wsFormat), m_dwType);
  switch (eCategory) {
    case FX_LOCALECATEGORY_Null:
      if (m_wsValue.IsEmpty())
        bRet = pFormat->FormatNull(wsFormat, &wsResult);
      break;
    case FX_LOCALECATEGORY_Zero:
      if (m_wsValue == L"0")
        bRet = pFormat->FormatZero(wsFormat, &wsResult);
      break;
    case FX_LOCALECATEGORY_Num:
      bRet = pFormat->FormatNum(m_wsValue, wsFormat, &wsResult);
      break;
    case FX_LOCALECATEGORY_Text:
      bRet = pFormat->FormatText(m_wsValue, wsFormat, &wsResult);
      break;
    case FX_LOCALECATEGORY_Date:
      bRet = pFormat->FormatDateTime(m_wsValue, wsFormat, FX_DATETIMETYPE_Date,
                                     &wsResult);
      break;
    case FX_LOCALECATEGORY_Time:
      bRet = pFormat->FormatDateTime(m_wsValue, wsFormat, FX_DATETIMETYPE_Time,
                                     &wsResult);
      break;
    case FX_LOCALECATEGORY_DateTime:
      bRet = pFormat->FormatDateTime(m_wsValue, wsFormat,
                                     FX_DATETIMETYPE_DateTime, &wsResult);
      break;
    default:
      wsResult = m_wsValue;
      bRet = true;
  }
  if (!bRet && (eCategory != FX_LOCALECATEGORY_Num ||
                eValueType != XFA_VALUEPICTURE_Display)) {
    wsResult = m_wsValue;
  }
  if (pLocale)
    m_pLocaleMgr->SetDefLocale(locale);

  return bRet;
}

bool CXFA_LocaleValue::ValidateCanonicalValue(const WideString& wsValue,
                                              uint32_t dwVType) {
  if (wsValue.IsEmpty())
    return true;

  CFX_DateTime dt;
  switch (dwVType) {
    case XFA_VT_DATE: {
      if (ValidateCanonicalDate(wsValue, &dt))
        return true;

      WideString wsDate;
      WideString wsTime;
      if (ValueSplitDateTime(wsValue, wsDate, wsTime) &&
          ValidateCanonicalDate(wsDate, &dt)) {
        return true;
      }
      return false;
    }
    case XFA_VT_TIME: {
      if (ValidateCanonicalTime(wsValue))
        return true;

      WideString wsDate;
      WideString wsTime;
      if (ValueSplitDateTime(wsValue, wsDate, wsTime) &&
          ValidateCanonicalTime(wsTime)) {
        return true;
      }
      return false;
    }
    case XFA_VT_DATETIME: {
      WideString wsDate, wsTime;
      if (ValueSplitDateTime(wsValue, wsDate, wsTime) &&
          ValidateCanonicalDate(wsDate, &dt) && ValidateCanonicalTime(wsTime)) {
        return true;
      }
    } break;
  }
  return true;
}

bool CXFA_LocaleValue::ValidateCanonicalDate(const WideString& wsDate,
                                             CFX_DateTime* unDate) {
  static const uint16_t LastDay[12] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};
  static const uint16_t wCountY = 4;
  static const uint16_t wCountM = 2;
  static const uint16_t wCountD = 2;
  int nLen = wsDate.GetLength();
  if (nLen < wCountY || nLen > wCountY + wCountM + wCountD + 2)
    return false;

  const bool bSymbol = wsDate.Contains(0x2D);
  uint16_t wYear = 0;
  uint16_t wMonth = 0;
  uint16_t wDay = 0;
  const wchar_t* pDate = wsDate.c_str();
  int nIndex = 0;
  int nStart = 0;
  while (pDate[nIndex] != '\0' && nIndex < wCountY) {
    if (!FXSYS_isDecimalDigit(pDate[nIndex]))
      return false;

    wYear = (pDate[nIndex] - '0') + wYear * 10;
    nIndex++;
  }
  if (bSymbol) {
    if (pDate[nIndex] != 0x2D)
      return false;
    nIndex++;
  }

  nStart = nIndex;
  while (pDate[nIndex] != '\0' && nIndex - nStart < wCountM && nIndex < nLen) {
    if (!FXSYS_isDecimalDigit(pDate[nIndex]))
      return false;

    wMonth = (pDate[nIndex] - '0') + wMonth * 10;
    nIndex++;
  }
  if (bSymbol) {
    if (pDate[nIndex] != 0x2D)
      return false;
    nIndex++;
  }

  nStart = nIndex;
  while (pDate[nIndex] != '\0' && nIndex - nStart < wCountD && nIndex < nLen) {
    if (!FXSYS_isDecimalDigit(pDate[nIndex]))
      return false;

    wDay = (pDate[nIndex] - '0') + wDay * 10;
    nIndex++;
  }
  if (nIndex != nLen)
    return false;
  if (wYear < 1900 || wYear > 2029)
    return false;
  if (wMonth < 1 || wMonth > 12)
    return wMonth == 0 && nLen == wCountY;
  if (wDay < 1)
    return wDay == 0 && (nLen == wCountY + wCountM);
  if (wMonth == 2) {
    if (wYear % 400 == 0 || (wYear % 100 != 0 && wYear % 4 == 0)) {
      if (wDay > 29)
        return false;
    } else if (wDay > 28) {
      return false;
    }
  } else if (wDay > LastDay[wMonth - 1]) {
    return false;
  }

  unDate->SetDate(wYear, static_cast<uint8_t>(wMonth),
                  static_cast<uint8_t>(wDay));
  return true;
}

bool CXFA_LocaleValue::ValidateCanonicalTime(const WideString& wsTime) {
  int nLen = wsTime.GetLength();
  if (nLen < 2)
    return false;

  const uint16_t wCountH = 2;
  const uint16_t wCountM = 2;
  const uint16_t wCountS = 2;
  const uint16_t wCountF = 3;
  const bool bSymbol = wsTime.Contains(':');
  uint16_t wHour = 0;
  uint16_t wMinute = 0;
  uint16_t wSecond = 0;
  uint16_t wFraction = 0;
  const wchar_t* pTime = wsTime.c_str();
  int nIndex = 0;
  int nStart = 0;
  while (nIndex - nStart < wCountH && pTime[nIndex]) {
    if (!FXSYS_isDecimalDigit(pTime[nIndex]))
      return false;
    wHour = pTime[nIndex] - '0' + wHour * 10;
    nIndex++;
  }
  if (bSymbol) {
    if (nIndex < nLen && pTime[nIndex] != ':')
      return false;
    nIndex++;
  }

  nStart = nIndex;
  while (nIndex - nStart < wCountM && nIndex < nLen && pTime[nIndex]) {
    if (!FXSYS_isDecimalDigit(pTime[nIndex]))
      return false;
    wMinute = pTime[nIndex] - '0' + wMinute * 10;
    nIndex++;
  }
  if (bSymbol) {
    if (nIndex < nLen && pTime[nIndex] != ':')
      return false;
    nIndex++;
  }
  nStart = nIndex;
  while (nIndex - nStart < wCountS && nIndex < nLen && pTime[nIndex]) {
    if (!FXSYS_isDecimalDigit(pTime[nIndex]))
      return false;
    wSecond = pTime[nIndex] - '0' + wSecond * 10;
    nIndex++;
  }
  auto pos = wsTime.Find('.');
  if (pos.has_value() && pos.value() != 0) {
    if (pTime[nIndex] != '.')
      return false;
    nIndex++;
    nStart = nIndex;
    while (nIndex - nStart < wCountF && nIndex < nLen && pTime[nIndex]) {
      if (!FXSYS_isDecimalDigit(pTime[nIndex]))
        return false;
      wFraction = pTime[nIndex] - '0' + wFraction * 10;
      nIndex++;
    }
  }
  if (nIndex < nLen) {
    if (pTime[nIndex] == 'Z') {
      nIndex++;
    } else if (pTime[nIndex] == '-' || pTime[nIndex] == '+') {
      int16_t nOffsetH = 0;
      int16_t nOffsetM = 0;
      nIndex++;
      nStart = nIndex;
      while (nIndex - nStart < wCountH && nIndex < nLen && pTime[nIndex]) {
        if (!FXSYS_isDecimalDigit(pTime[nIndex]))
          return false;
        nOffsetH = pTime[nIndex] - '0' + nOffsetH * 10;
        nIndex++;
      }
      if (bSymbol) {
        if (nIndex < nLen && pTime[nIndex] != ':')
          return false;
        nIndex++;
      }
      nStart = nIndex;
      while (nIndex - nStart < wCountM && nIndex < nLen && pTime[nIndex]) {
        if (!FXSYS_isDecimalDigit(pTime[nIndex]))
          return false;
        nOffsetM = pTime[nIndex] - '0' + nOffsetM * 10;
        nIndex++;
      }
      if (nOffsetH > 12 || nOffsetM >= 60)
        return false;
    }
  }
  return nIndex == nLen && wHour < 24 && wMinute < 60 && wSecond < 60 &&
         wFraction <= 999;
}

bool CXFA_LocaleValue::ParsePatternValue(const WideString& wsValue,
                                         const WideString& wsPattern,
                                         IFX_Locale* pLocale) {
  IFX_Locale* locale = m_pLocaleMgr->GetDefLocale();
  if (pLocale)
    m_pLocaleMgr->SetDefLocale(pLocale);

  auto pFormat = pdfium::MakeUnique<CFGAS_FormatString>(m_pLocaleMgr);
  std::vector<WideString> wsPatterns;
  pFormat->SplitFormatString(wsPattern, &wsPatterns);
  bool bRet = false;
  int32_t iCount = pdfium::CollectionSize<int32_t>(wsPatterns);
  for (int32_t i = 0; i < iCount && !bRet; i++) {
    WideString wsFormat = wsPatterns[i];
    switch (ValueCategory(pFormat->GetCategory(wsFormat), m_dwType)) {
      case FX_LOCALECATEGORY_Null:
        bRet = pFormat->ParseNull(wsValue, wsFormat);
        if (bRet)
          m_wsValue.clear();
        break;
      case FX_LOCALECATEGORY_Zero:
        bRet = pFormat->ParseZero(wsValue, wsFormat);
        if (bRet)
          m_wsValue = L"0";
        break;
      case FX_LOCALECATEGORY_Num: {
        WideString fNum;
        bRet = pFormat->ParseNum(wsValue, wsFormat, &fNum);
        if (bRet)
          m_wsValue = fNum;
        break;
      }
      case FX_LOCALECATEGORY_Text:
        bRet = pFormat->ParseText(wsValue, wsFormat, &m_wsValue);
        break;
      case FX_LOCALECATEGORY_Date: {
        CFX_DateTime dt;
        bRet = ValidateCanonicalDate(wsValue, &dt);
        if (!bRet) {
          bRet = pFormat->ParseDateTime(wsValue, wsFormat, FX_DATETIMETYPE_Date,
                                        &dt);
        }
        if (bRet)
          SetDate(dt);
        break;
      }
      case FX_LOCALECATEGORY_Time: {
        CFX_DateTime dt;
        bRet = pFormat->ParseDateTime(wsValue, wsFormat, FX_DATETIMETYPE_Time,
                                      &dt);
        if (bRet)
          SetTime(dt);
        break;
      }
      case FX_LOCALECATEGORY_DateTime: {
        CFX_DateTime dt;
        bRet = pFormat->ParseDateTime(wsValue, wsFormat,
                                      FX_DATETIMETYPE_DateTime, &dt);
        if (bRet)
          SetDateTime(dt);
        break;
      }
      default:
        m_wsValue = wsValue;
        bRet = true;
        break;
    }
  }
  if (!bRet)
    m_wsValue = wsValue;

  if (pLocale)
    m_pLocaleMgr->SetDefLocale(locale);

  return bRet;
}

void CXFA_LocaleValue::GetNumericFormat(WideString& wsFormat,
                                        int32_t nIntLen,
                                        int32_t nDecLen) {
  ASSERT(wsFormat.IsEmpty());
  ASSERT(nIntLen >= -1 && nDecLen >= -1);

  int32_t nTotalLen = (nIntLen >= 0 ? nIntLen : 2) + 1 +
                      (nDecLen >= 0 ? nDecLen : 2) + (nDecLen == 0 ? 0 : 1);
  wchar_t* lpBuf = wsFormat.GetBuffer(nTotalLen);
  int32_t nPos = 0;
  lpBuf[nPos++] = L's';

  if (nIntLen == -1) {
    lpBuf[nPos++] = L'z';
    lpBuf[nPos++] = L'*';
  } else {
    while (nIntLen) {
      lpBuf[nPos++] = L'z';
      nIntLen--;
    }
  }
  if (nDecLen != 0) {
    lpBuf[nPos++] = L'.';
  }
  if (nDecLen == -1) {
    lpBuf[nPos++] = L'z';
    lpBuf[nPos++] = L'*';
  } else {
    while (nDecLen) {
      lpBuf[nPos++] = L'z';
      nDecLen--;
    }
  }
  wsFormat.ReleaseBuffer(nTotalLen);
}

bool CXFA_LocaleValue::ValidateNumericTemp(const WideString& wsNumeric,
                                           const WideString& wsFormat,
                                           IFX_Locale* pLocale) {
  if (wsFormat.IsEmpty() || wsNumeric.IsEmpty())
    return true;

  const wchar_t* pNum = wsNumeric.c_str();
  const wchar_t* pFmt = wsFormat.c_str();
  int32_t n = 0;
  int32_t nf = 0;
  wchar_t c = pNum[n];
  wchar_t cf = pFmt[nf];
  if (cf == L's') {
    if (c == L'-' || c == L'+')
      ++n;
    ++nf;
  }

  bool bLimit = true;
  int32_t nCount = wsNumeric.GetLength();
  int32_t nCountFmt = wsFormat.GetLength();
  while (n < nCount && (bLimit ? nf < nCountFmt : true) &&
         FXSYS_isDecimalDigit(c = pNum[n])) {
    if (bLimit == true) {
      if ((cf = pFmt[nf]) == L'*')
        bLimit = false;
      else if (cf == L'z')
        nf++;
      else
        return false;
    }
    n++;
  }
  if (n == nCount)
    return true;
  if (nf == nCountFmt)
    return false;

  while (nf < nCountFmt && (cf = pFmt[nf]) != L'.') {
    ASSERT(cf == L'z' || cf == L'*');
    ++nf;
  }

  WideString wsDecimalSymbol;
  if (pLocale)
    wsDecimalSymbol = pLocale->GetNumbericSymbol(FX_LOCALENUMSYMBOL_Decimal);
  else
    wsDecimalSymbol = WideString(L'.');

  if (pFmt[nf] != L'.')
    return false;
  if (wsDecimalSymbol != WideStringView(c) && c != L'.')
    return false;

  ++nf;
  ++n;
  bLimit = true;
  while (n < nCount && (bLimit ? nf < nCountFmt : true) &&
         FXSYS_isDecimalDigit(c = pNum[n])) {
    if (bLimit == true) {
      if ((cf = pFmt[nf]) == L'*')
        bLimit = false;
      else if (cf == L'z')
        nf++;
      else
        return false;
    }
    n++;
  }
  return n == nCount;
}

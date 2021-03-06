// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_FX_ARABIC_H_
#define CORE_FXCRT_FX_ARABIC_H_

#include <vector>

#include "core/fxcrt/cfx_char.h"
#include "core/fxcrt/fx_system.h"

namespace pdfium {
namespace arabic {

wchar_t GetFormChar(wchar_t wch, wchar_t prev, wchar_t next);
wchar_t GetFormChar(const CFX_Char* cur,
                    const CFX_Char* prev,
                    const CFX_Char* next);

}  // namespace arabic
}  // namespace pdfium

wchar_t FX_GetArabicFromShaddaTable(wchar_t shadda);

#endif  // CORE_FXCRT_FX_ARABIC_H_

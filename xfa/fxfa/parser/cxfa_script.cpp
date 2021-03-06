// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_script.h"

#include "fxjs/xfa/cjx_script.h"
#include "third_party/base/ptr_util.h"

namespace {

const CXFA_Node::PropertyData kPropertyData[] = {
    {XFA_Element::Exclude, 1, 0},
    {XFA_Element::CurrentPage, 1, 0},
    {XFA_Element::RunScripts, 1, 0},
    {XFA_Element::Unknown, 0, 0}};
const CXFA_Node::AttributeData kAttributeData[] = {
    {XFA_Attribute::Id, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Name, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Use, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::ContentType, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::RunAt, XFA_AttributeType::Enum,
     (void*)XFA_AttributeEnum::Client},
    {XFA_Attribute::Binding, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Usehref, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Desc, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Lock, XFA_AttributeType::Integer, (void*)0},
    {XFA_Attribute::Unknown, XFA_AttributeType::Integer, nullptr}};

constexpr wchar_t kName[] = L"script";

}  // namespace

CXFA_Script::CXFA_Script(CXFA_Document* doc, XFA_PacketType packet)
    : CXFA_Node(
          doc,
          packet,
          (XFA_XDPPACKET_Config | XFA_XDPPACKET_Template | XFA_XDPPACKET_Form),
          XFA_ObjectType::ContentNode,
          XFA_Element::Script,
          kPropertyData,
          kAttributeData,
          kName,
          pdfium::MakeUnique<CJX_Script>(this)) {}

CXFA_Script::~CXFA_Script() {}

CXFA_Script::Type CXFA_Script::GetContentType() {
  Optional<WideString> cData =
      JSObject()->TryCData(XFA_Attribute::ContentType, false);
  if (!cData || *cData == L"application/x-formcalc")
    return Type::Formcalc;
  if (*cData == L"application/x-javascript")
    return Type::Javascript;
  return Type::Unknown;
}

XFA_AttributeEnum CXFA_Script::GetRunAt() {
  return JSObject()->GetEnum(XFA_Attribute::RunAt);
}

WideString CXFA_Script::GetExpression() {
  return JSObject()->GetContent(false);
}

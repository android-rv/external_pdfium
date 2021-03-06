// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_permissions.h"

namespace {

const CXFA_Node::PropertyData kPropertyData[] = {
    {XFA_Element::ModifyAnnots, 1, 0},
    {XFA_Element::ContentCopy, 1, 0},
    {XFA_Element::FormFieldFilling, 1, 0},
    {XFA_Element::Change, 1, 0},
    {XFA_Element::AccessibleContent, 1, 0},
    {XFA_Element::Print, 1, 0},
    {XFA_Element::PlaintextMetadata, 1, 0},
    {XFA_Element::PrintHighQuality, 1, 0},
    {XFA_Element::DocumentAssembly, 1, 0},
    {XFA_Element::Unknown, 0, 0}};
const CXFA_Node::AttributeData kAttributeData[] = {
    {XFA_Attribute::Desc, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Lock, XFA_AttributeType::Integer, (void*)0},
    {XFA_Attribute::Unknown, XFA_AttributeType::Integer, nullptr}};

constexpr wchar_t kName[] = L"permissions";

}  // namespace

CXFA_Permissions::CXFA_Permissions(CXFA_Document* doc, XFA_PacketType packet)
    : CXFA_Node(doc,
                packet,
                XFA_XDPPACKET_Config,
                XFA_ObjectType::Node,
                XFA_Element::Permissions,
                kPropertyData,
                kAttributeData,
                kName) {}

CXFA_Permissions::~CXFA_Permissions() {}

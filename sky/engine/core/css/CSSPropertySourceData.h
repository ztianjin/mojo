/*
 * Copyright (c) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SKY_ENGINE_CORE_CSS_CSSPROPERTYSOURCEDATA_H_
#define SKY_ENGINE_CORE_CSS_CSSPROPERTYSOURCEDATA_H_

#include "sky/engine/platform/heap/Handle.h"
#include "sky/engine/wtf/Forward.h"
#include "sky/engine/wtf/RefCounted.h"
#include "sky/engine/wtf/Vector.h"
#include "sky/engine/wtf/text/WTFString.h"

namespace blink {

class StyleRuleBase;

struct SourceRange {
    ALLOW_ONLY_INLINE_ALLOCATION();
public:
    SourceRange();
    SourceRange(unsigned start, unsigned end);
    unsigned length() const;

    unsigned start;
    unsigned end;
};

struct CSSPropertySourceData {
    ALLOW_ONLY_INLINE_ALLOCATION();
public:
    CSSPropertySourceData(const String& name, const String& value, bool disabled, bool parsedOk, const SourceRange& range);
    CSSPropertySourceData(const CSSPropertySourceData& other);
    CSSPropertySourceData();

    String toString() const;
    unsigned hash() const;

    String name;
    String value;
    bool disabled;
    bool parsedOk;
    SourceRange range;
};

struct CSSStyleSourceData : public RefCounted<CSSStyleSourceData> {
    static PassRefPtr<CSSStyleSourceData> create()
    {
        return adoptRef(new CSSStyleSourceData());
    }

    Vector<CSSPropertySourceData> propertyData;
};

struct CSSRuleSourceData;
typedef Vector<RefPtr<CSSRuleSourceData> > RuleSourceDataList;
typedef Vector<SourceRange> SelectorRangeList;

struct CSSRuleSourceData : public RefCounted<CSSRuleSourceData> {
    enum Type {
        UNKNOWN_RULE = 0,
        STYLE_RULE = 1,
        MEDIA_RULE = 3,
        FONT_FACE_RULE = 4,
        SUPPORTS_RULE = 8,
        FILTER_RULE = 9
    };

    static PassRefPtr<CSSRuleSourceData> create(Type type)
    {
        return adoptRef(new CSSRuleSourceData(type));
    }

    static PassRefPtr<CSSRuleSourceData> createUnknown()
    {
        return adoptRef(new CSSRuleSourceData(UNKNOWN_RULE));
    }

    CSSRuleSourceData(Type type)
        : type(type)
    {
        if (type == STYLE_RULE || type == FONT_FACE_RULE)
            styleSourceData = CSSStyleSourceData::create();
    }

    Type type;

    // Range of the selector list in the enclosing source.
    SourceRange ruleHeaderRange;

    // Range of the rule body (e.g. style text for style rules) in the enclosing source.
    SourceRange ruleBodyRange;

    // Only for CSSStyleRules.
    SelectorRangeList selectorRanges;

    // Only for CSSStyleRules, CSSFontFaceRules.
    RefPtr<CSSStyleSourceData> styleSourceData;

    // Only for CSSMediaRules.
    RuleSourceDataList childRules;
};

} // namespace blink

WTF_ALLOW_MOVE_AND_INIT_WITH_MEM_FUNCTIONS(blink::SourceRange);
WTF_ALLOW_MOVE_AND_INIT_WITH_MEM_FUNCTIONS(blink::CSSPropertySourceData);

#endif  // SKY_ENGINE_CORE_CSS_CSSPROPERTYSOURCEDATA_H_

/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SKY_ENGINE_WEB_CHROMECLIENTIMPL_H_
#define SKY_ENGINE_WEB_CHROMECLIENTIMPL_H_

#include "sky/engine/core/page/ChromeClient.h"
#include "sky/engine/platform/weborigin/KURL.h"
#include "sky/engine/public/platform/WebColor.h"
#include "sky/engine/public/web/WebNavigationPolicy.h"
#include "sky/engine/wtf/PassOwnPtr.h"

namespace blink {
class ColorChooser;
class ColorChooserClient;
class Element;
class Event;
class KeyboardEvent;
class RenderBox;
class DateTimeChooser;
class DateTimeChooserClient;
class WebViewImpl;

// Handles window-level notifications from WebCore on behalf of a WebView.
class ChromeClientImpl final : public ChromeClient {
public:
    explicit ChromeClientImpl(WebViewImpl* webView);
    virtual ~ChromeClientImpl();

    virtual void* webView() const override;

    // ChromeClient methods:
    virtual void setWindowRect(const FloatRect&) override;
    virtual FloatRect windowRect() override;
    virtual void focus() override;
    virtual bool canTakeFocus(FocusType) override;
    virtual void takeFocus(FocusType) override;
    virtual void focusedNodeChanged(Node*) override;
    virtual void focusedFrameChanged(LocalFrame*) override;

    virtual bool shouldReportDetailedMessageForSource(const WTF::String&) override;
    virtual void addMessageToConsole(
        LocalFrame*, MessageSource, MessageLevel,
        const WTF::String& message, unsigned lineNumber,
        const WTF::String& sourceID, const WTF::String& stackTrace) override;
    virtual void scheduleVisualUpdate() override;
    virtual IntRect rootViewToScreen(const IntRect&) const override;
    virtual WebScreenInfo screenInfo() const override;

    // ChromeClient methods:
    virtual String acceptLanguages() override;

private:
    WebNavigationPolicy getNavigationPolicy();

    WebViewImpl* m_webView;  // weak pointer
};

} // namespace blink

#endif  // SKY_ENGINE_WEB_CHROMECLIENTIMPL_H_

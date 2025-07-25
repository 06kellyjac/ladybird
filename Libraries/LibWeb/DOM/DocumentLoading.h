/*
 * Copyright (c) 2020, Andreas Kling <andreas@ladybird.org>
 * Copyright (c) 2023, Aliaksandr Kalenik <kalenik.aliaksandr@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/DOM/Document.h>
#include <LibWeb/Fetch/Infrastructure/HTTP/Responses.h>

namespace Web {

bool build_xml_document(DOM::Document& document, ByteBuffer const& data, Optional<String> content_encoding);
GC::Ptr<DOM::Document> load_document(HTML::NavigationParams const& navigation_params);
bool can_load_document_with_type(MimeSniff::MimeType const&);

// https://html.spec.whatwg.org/multipage/document-lifecycle.html#read-ua-inline
template<typename MutateDocument>
GC::Ref<DOM::Document> create_document_for_inline_content(GC::Ptr<HTML::Navigable> navigable, Optional<String> navigation_id, HTML::UserNavigationInvolvement user_involvement, MutateDocument mutate_document)
{
    auto& vm = navigable->vm();
    VERIFY(navigable->active_document());

    // 1. Let origin be a new opaque origin.
    auto origin = URL::Origin::create_opaque();

    // 2. Let coop be a new opener policy.
    auto coop = HTML::OpenerPolicy {};

    // 3. Let coopEnforcementResult be a new opener policy enforcement result with
    //    url: response's URL
    //    origin: origin
    //    opener policy: coop
    HTML::OpenerPolicyEnforcementResult coop_enforcement_result {
        .url = URL::about_error(), // AD-HOC
        .origin = origin,
        .opener_policy = coop
    };

    // 4. Let navigationParams be a new navigation params with
    //    id: navigationId
    //    navigable: navigable
    //    request: null
    //    response: a new response
    //    origin: origin
    //    fetch controller: null
    //    commit early hints: null
    //    COOP enforcement result: coopEnforcementResult
    //    reserved environment: null
    //    policy container: a new policy container
    //    final sandboxing flag set: an empty set
    //    opener policy: coop
    //    FIXME: navigation timing type: navTimingType
    //    about base URL: null
    //    user involvement: userInvolvement
    auto response = Fetch::Infrastructure::Response::create(vm);
    response->url_list().append(URL::about_error()); // AD-HOC: https://github.com/whatwg/html/issues/9122
    auto navigation_params = vm.heap().allocate<HTML::NavigationParams>(
        move(navigation_id),
        navigable,
        nullptr,
        response,
        nullptr,
        nullptr,
        move(coop_enforcement_result),
        nullptr,
        move(origin),
        vm.heap().allocate<HTML::PolicyContainer>(vm.heap()),
        HTML::SandboxingFlagSet {},
        move(coop),
        OptionalNone {},
        user_involvement);

    // 5. Let document be the result of creating and initializing a Document object given "html", "text/html", and navigationParams.
    auto document = DOM::Document::create_and_initialize(DOM::Document::Type::HTML, "text/html"_string, navigation_params).release_value_but_fixme_should_propagate_errors();

    // 6. Either associate document with a custom rendering that is not rendered using the normal Document rendering rules, or mutate document until it represents the content the
    //    user agent wants to render.
    mutate_document(*document);

    // 7. Return document.
    return document;
}

}

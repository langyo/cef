// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/ceftests/test_util.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "include/base/cef_callback.h"
#include "include/cef_base.h"
#include "include/cef_command_line.h"
#include "include/cef_request_context_handler.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "tests/gtest/include/gtest/gtest.h"
#include "tests/shared/common/client_switches.h"
#include "tests/shared/common/string_util.h"

void TestMapEqual(const CefRequest::HeaderMap& map1,
                  const CefRequest::HeaderMap& map2,
                  bool allowExtras) {
  if (!allowExtras) {
    EXPECT_EQ(map1.size(), map2.size());
  }

  TestMapNoDuplicates(map1);
  TestMapNoDuplicates(map2);

  CefRequest::HeaderMap::const_iterator it1, it2;

  for (it1 = map1.begin(); it1 != map1.end(); ++it1) {
    bool found = false;
    std::string name1 = client::AsciiStrToLower(it1->first);
    for (it2 = map2.begin(); it2 != map2.end(); ++it2) {
      std::string name2 = client::AsciiStrToLower(it2->first);
      if (name1 == name2 && it1->second == it2->second) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << "No entry for " << it1->first.ToString() << ": "
                       << it1->second.ToString();
  }
}

void TestMapNoDuplicates(const CefRequest::HeaderMap& map) {
  CefRequest::HeaderMap::const_iterator it1 = map.begin();
  for (; it1 != map.end(); ++it1) {
    CefRequest::HeaderMap::const_iterator it2 = it1;
    for (++it2; it2 != map.end(); ++it2) {
      EXPECT_FALSE(it1->first == it2->first && it1->second == it2->second)
          << "Duplicate entry for " << it1->first.ToString() << ": "
          << it1->second.ToString();
    }
  }
}

void TestPostDataElementEqual(CefRefPtr<CefPostDataElement> elem1,
                              CefRefPtr<CefPostDataElement> elem2) {
  EXPECT_TRUE(elem1.get());
  EXPECT_TRUE(elem2.get());

  EXPECT_EQ(elem1->GetType(), elem2->GetType());
  switch (elem1->GetType()) {
    case PDE_TYPE_BYTES: {
      EXPECT_EQ(elem1->GetBytesCount(), elem2->GetBytesCount());
      size_t bytesCt = elem1->GetBytesCount();
      char* buff1 = new char[bytesCt];
      char* buff2 = new char[bytesCt];
      elem1->GetBytes(bytesCt, buff1);
      elem2->GetBytes(bytesCt, buff2);
      EXPECT_TRUE(!memcmp(buff1, buff2, bytesCt));
      delete[] buff1;
      delete[] buff2;
    } break;
    case PDE_TYPE_FILE:
      EXPECT_EQ(elem1->GetFile(), elem2->GetFile());
      break;
    default:
      break;
  }
}

void TestPostDataEqual(CefRefPtr<CefPostData> postData1,
                       CefRefPtr<CefPostData> postData2) {
  EXPECT_TRUE(postData1.get());
  EXPECT_TRUE(postData2.get());

  EXPECT_EQ(postData1->GetElementCount(), postData2->GetElementCount());

  CefPostData::ElementVector ev1, ev2;
  postData1->GetElements(ev1);
  postData1->GetElements(ev2);
  ASSERT_EQ(ev1.size(), ev2.size());

  CefPostData::ElementVector::const_iterator it1 = ev1.begin();
  CefPostData::ElementVector::const_iterator it2 = ev2.begin();
  for (; it1 != ev1.end() && it2 != ev2.end(); ++it1, ++it2) {
    TestPostDataElementEqual((*it1), (*it2));
  }
}

void TestRequestEqual(CefRefPtr<CefRequest> request1,
                      CefRefPtr<CefRequest> request2,
                      bool allowExtras) {
  EXPECT_TRUE(request1.get());
  EXPECT_TRUE(request2.get());

  EXPECT_STREQ(request1->GetURL().ToString().c_str(),
               request2->GetURL().ToString().c_str());
  EXPECT_STREQ(request1->GetMethod().ToString().c_str(),
               request2->GetMethod().ToString().c_str());

  EXPECT_STREQ(request1->GetReferrerURL().ToString().c_str(),
               request2->GetReferrerURL().ToString().c_str());
  EXPECT_EQ(request1->GetReferrerPolicy(), request2->GetReferrerPolicy());

  CefRequest::HeaderMap headers1, headers2;
  request1->GetHeaderMap(headers1);
  request2->GetHeaderMap(headers2);
  TestMapEqual(headers1, headers2, allowExtras);

  CefRefPtr<CefPostData> postData1 = request1->GetPostData();
  CefRefPtr<CefPostData> postData2 = request2->GetPostData();
  EXPECT_EQ(!!(postData1.get()), !!(postData2.get()));
  if (postData1.get() && postData2.get()) {
    TestPostDataEqual(postData1, postData2);
  }
}

void TestResponseEqual(CefRefPtr<CefResponse> response1,
                       CefRefPtr<CefResponse> response2,
                       bool allowExtras) {
  EXPECT_TRUE(response1.get());
  EXPECT_TRUE(response2.get());

  EXPECT_EQ(response1->GetStatus(), response2->GetStatus());
  EXPECT_STREQ(response1->GetStatusText().ToString().c_str(),
               response2->GetStatusText().ToString().c_str());
  EXPECT_STREQ(response1->GetMimeType().ToString().c_str(),
               response2->GetMimeType().ToString().c_str());

  CefRequest::HeaderMap headers1, headers2;
  response1->GetHeaderMap(headers1);
  response2->GetHeaderMap(headers2);
  TestMapEqual(headers1, headers2, allowExtras);
}

void TestBinaryEqual(CefRefPtr<CefBinaryValue> val1,
                     CefRefPtr<CefBinaryValue> val2) {
  EXPECT_TRUE(val1.get());
  EXPECT_TRUE(val2.get());

  EXPECT_TRUE(val1->IsEqual(val2));
  EXPECT_TRUE(val2->IsEqual(val1));

  size_t data_size = val1->GetSize();
  EXPECT_EQ(data_size, val2->GetSize());

  EXPECT_GT(data_size, (size_t)0);

  char* data1 = new char[data_size + 1];
  char* data2 = new char[data_size + 1];

  EXPECT_EQ(data_size, val1->GetData(data1, data_size, 0));
  data1[data_size] = 0;
  EXPECT_EQ(data_size, val2->GetData(data2, data_size, 0));
  data2[data_size] = 0;

  EXPECT_STREQ(data1, data2);

  delete[] data1;
  delete[] data2;
}

void TestDictionaryEqual(CefRefPtr<CefDictionaryValue> val1,
                         CefRefPtr<CefDictionaryValue> val2) {
  EXPECT_TRUE(val1.get());
  EXPECT_TRUE(val2.get());

  EXPECT_TRUE(val1->IsEqual(val2));
  EXPECT_TRUE(val2->IsEqual(val1));

  EXPECT_EQ(val1->GetSize(), val2->GetSize());

  CefDictionaryValue::KeyList keys;
  EXPECT_TRUE(val1->GetKeys(keys));

  CefDictionaryValue::KeyList::const_iterator it = keys.begin();
  for (; it != keys.end(); ++it) {
    CefString key = *it;
    EXPECT_TRUE(val2->HasKey(key));
    CefValueType type = val1->GetType(key);
    EXPECT_EQ(type, val2->GetType(key));
    switch (type) {
      case VTYPE_INVALID:
      case VTYPE_NULL:
        break;
      case VTYPE_BOOL:
        EXPECT_EQ(val1->GetBool(key), val2->GetBool(key));
        break;
      case VTYPE_INT:
        EXPECT_EQ(val1->GetInt(key), val2->GetInt(key));
        break;
      case VTYPE_DOUBLE:
        EXPECT_EQ(val1->GetDouble(key), val2->GetDouble(key));
        break;
      case VTYPE_STRING:
        EXPECT_EQ(val1->GetString(key), val2->GetString(key));
        break;
      case VTYPE_BINARY:
        TestBinaryEqual(val1->GetBinary(key), val2->GetBinary(key));
        break;
      case VTYPE_DICTIONARY:
        TestDictionaryEqual(val1->GetDictionary(key), val2->GetDictionary(key));
        break;
      case VTYPE_LIST:
        TestListEqual(val1->GetList(key), val2->GetList(key));
        break;
      case VTYPE_NUM_VALUES:
        NOTREACHED();
        break;
    }
  }
}

void TestListEqual(CefRefPtr<CefListValue> val1, CefRefPtr<CefListValue> val2) {
  EXPECT_TRUE(val1.get());
  EXPECT_TRUE(val2.get());

  EXPECT_TRUE(val1->IsEqual(val2));
  EXPECT_TRUE(val2->IsEqual(val1));

  size_t size = val1->GetSize();
  EXPECT_EQ(size, val2->GetSize());

  for (size_t i = 0; i < size; ++i) {
    CefValueType type = val1->GetType(i);
    EXPECT_EQ(type, val2->GetType(i));
    switch (type) {
      case VTYPE_INVALID:
      case VTYPE_NULL:
        break;
      case VTYPE_BOOL:
        EXPECT_EQ(val1->GetBool(i), val2->GetBool(i));
        break;
      case VTYPE_INT:
        EXPECT_EQ(val1->GetInt(i), val2->GetInt(i));
        break;
      case VTYPE_DOUBLE:
        EXPECT_EQ(val1->GetDouble(i), val2->GetDouble(i));
        break;
      case VTYPE_STRING:
        EXPECT_EQ(val1->GetString(i), val2->GetString(i));
        break;
      case VTYPE_BINARY:
        TestBinaryEqual(val1->GetBinary(i), val2->GetBinary(i));
        break;
      case VTYPE_DICTIONARY:
        TestDictionaryEqual(val1->GetDictionary(i), val2->GetDictionary(i));
        break;
      case VTYPE_LIST:
        TestListEqual(val1->GetList(i), val2->GetList(i));
        break;
      case VTYPE_NUM_VALUES:
        NOTREACHED();
        break;
    }
  }
}

void TestProcessMessageEqual(CefRefPtr<CefProcessMessage> val1,
                             CefRefPtr<CefProcessMessage> val2) {
  EXPECT_TRUE(val1.get());
  EXPECT_TRUE(val2.get());
  EXPECT_EQ(val1->GetName(), val2->GetName());

  TestListEqual(val1->GetArgumentList(), val2->GetArgumentList());
}

void TestStringVectorEqual(const std::vector<CefString>& val1,
                           const std::vector<CefString>& val2) {
  EXPECT_EQ(val1.size(), val2.size());
  if (val1.size() != val2.size()) {
    return;
  }

  for (size_t i = 0; i < val1.size(); ++i) {
    EXPECT_STREQ(val1[i].ToString().c_str(), val2[i].ToString().c_str());
  }
}

bool TestOldResourceAPI() {
  static bool state = []() {
    return CefCommandLine::GetGlobalCommandLine()->HasSwitch(
        "test-old-resource-api");
  }();
  return state;
}

bool UseViewsGlobal() {
  static bool use_views = []() {
    return CefCommandLine::GetGlobalCommandLine()->HasSwitch(
        client::switches::kUseViews);
  }();
  return use_views;
}

bool UseAlloyStyleBrowserGlobal() {
  static bool use_alloy_style = []() {
    return CefCommandLine::GetGlobalCommandLine()->HasSwitch(
        client::switches::kUseAlloyStyle);
  }();
  return use_alloy_style;
}

bool UseAlloyStyleWindowGlobal() {
  static bool use_alloy_style = []() {
    auto command_line = CefCommandLine::GetGlobalCommandLine();
    return command_line->HasSwitch(client::switches::kUseAlloyStyle) &&
           !command_line->HasSwitch(client::switches::kUseChromeStyleWindow);
  }();
  return use_alloy_style;
}

std::string ComputeViewsWindowTitle(CefRefPtr<CefWindow> window,
                                    CefRefPtr<CefBrowserView> browser_view) {
  std::string title = "CefTest - Views - ";
  std::string window_style =
      window->GetRuntimeStyle() == CEF_RUNTIME_STYLE_CHROME ? "Chrome"
                                                            : "Alloy";
  title += window_style + " Window";
  if (browser_view) {
    std::string browser_style =
        browser_view->GetRuntimeStyle() == CEF_RUNTIME_STYLE_CHROME ? "Chrome"
                                                                    : "Alloy";
    title += " - " + browser_style + " BrowserView";
  }
  return title;
}

std::string ComputeNativeWindowTitle(bool use_alloy_style) {
  std::string title = "CefTest - Native - ";
  title += use_alloy_style ? "Alloy Browser" : "Chrome Browser";
  return title;
}

bool IsBFCacheEnabled() {
  static bool state = []() {
    const std::string& value =
        CefCommandLine::GetGlobalCommandLine()->GetSwitchValue(
            "disable-features");
    return value.find("BackForwardCache") == std::string::npos;
  }();
  return state;
}

bool IsSameSiteBFCacheEnabled() {
  // Same-site BFCache is enabled by default starting in M101 and does not
  // have a separate configuration flag.
  return IsBFCacheEnabled();
}

bool IgnoreURL(const std::string& url) {
  return url.find("/favicon.ico") != std::string::npos;
}

std::optional<int> GetConfiguredTestTimeout(int timeout_ms) {
  static std::optional<double> multiplier = []() -> std::optional<double> {
    auto command_line = CefCommandLine::GetGlobalCommandLine();
    if (command_line->HasSwitch("disable-test-timeout")) {
      return std::nullopt;
    }
    const std::string& sval =
        command_line->GetSwitchValue("test-timeout-multiplier");
    if (!sval.empty()) {
      if (double dval = std::atof(sval.c_str())) {
        return dval;
      }
    }
    return 2.0;
  }();

  if (!multiplier) {
    // Test timeout is disabled.
    return std::nullopt;
  }

  return static_cast<int>(
      std::round(static_cast<double>(timeout_ms) * (*multiplier)));
}

void SendMouseClickEvent(CefRefPtr<CefBrowser> browser,
                         const CefMouseEvent& mouse_event,
                         cef_mouse_button_type_t mouse_button_type) {
  auto host = browser->GetHost();
  CefPostDelayedTask(TID_UI,
                     base::BindOnce(&CefBrowserHost::SendMouseClickEvent, host,
                                    mouse_event, mouse_button_type, false, 1),
                     50);
  CefPostDelayedTask(TID_UI,
                     base::BindOnce(&CefBrowserHost::SendMouseClickEvent, host,
                                    mouse_event, mouse_button_type, true, 1),
                     100);
}

void GrantPopupPermission(CefRefPtr<CefRequestContext> request_context,
                          const std::string& parent_url) {
  static bool test_website_setting = []() {
    return CefCommandLine::GetGlobalCommandLine()->HasSwitch(
        "test-website-setting");
  }();

  // The below calls are equivalent.
  // NOTE: If the popup allow functionality stops working, debug the code in
  // components/blocked_content/popup_blocker.cc
  if (test_website_setting) {
    auto value = CefValue::Create();
    value->SetInt(CEF_CONTENT_SETTING_VALUE_ALLOW);
    request_context->SetWebsiteSetting(parent_url, parent_url,
                                       CEF_CONTENT_SETTING_TYPE_POPUPS, value);
  } else {
    request_context->SetContentSetting(parent_url, parent_url,
                                       CEF_CONTENT_SETTING_TYPE_POPUPS,
                                       CEF_CONTENT_SETTING_VALUE_ALLOW);
  }
}

void CreateTestRequestContext(TestRequestContextMode mode,
                              const std::string& cache_path,
                              RCInitCallback init_callback) {
  DCHECK(!init_callback.is_null());
  EXPECT_TRUE(cache_path.empty() || mode == TEST_RC_MODE_CUSTOM_WITH_HANDLER);

  if (mode == TEST_RC_MODE_NONE || mode == TEST_RC_MODE_GLOBAL) {
    // Global contexts are initialized synchronously during startup, so we can
    // execute the callback immediately.
    CefRefPtr<CefRequestContext> request_context;
    if (mode == TEST_RC_MODE_GLOBAL) {
      request_context = CefRequestContext::GetGlobalContext();
    }

    CefPostTask(TID_UI,
                base::BindOnce(std::move(init_callback), request_context));
    return;
  }

  class Handler : public CefRequestContextHandler {
   public:
    explicit Handler(RCInitCallback init_callback)
        : init_callback_(std::move(init_callback)) {}

    void OnRequestContextInitialized(
        CefRefPtr<CefRequestContext> request_context) override {
      CEF_REQUIRE_UI_THREAD();
      std::move(init_callback_).Run(request_context);
    }

   private:
    RCInitCallback init_callback_;
    IMPLEMENT_REFCOUNTING(Handler);
  };

  CefRefPtr<CefRequestContextHandler> rc_handler =
      new Handler(std::move(init_callback));

  if (mode == TEST_RC_MODE_CUSTOM_WITH_HANDLER) {
    CefRequestContextSettings settings;
    if (!cache_path.empty()) {
      CefString(&settings.cache_path) = cache_path;
    }
    CefRequestContext::CreateContext(settings, rc_handler);
  } else {
    CefRequestContext::CreateContext(CefRequestContext::GetGlobalContext(),
                                     rc_handler);
  }
}

CefTime CefTimeFrom(CefBaseTime value) {
  CefTime time;
  cef_time_from_basetime(value, &time);
  return time;
}

CefBaseTime CefBaseTimeFrom(const CefTime& value) {
  cef_basetime_t temp;
  cef_time_to_basetime(&value, &temp);
  return CefBaseTime(temp);
}

// Copyright (c) 2014 Jefry Tedjokusumo <jtg_gg@yahoo.com.sg>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell co
// pies of the Software, and to permit persons to whom the Software is furnished
//  to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in al
// l copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM
// PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNES
// S FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
//  OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WH
// ETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/nw/src/browser/native_window.h"
#include "content/nw/src/nw_package.h"
#include "content/nw/src/nw_shell.h"

#include "ui/gfx/image/image.h"
#include "base/strings/utf_string_conversions.h"
#include "content/nw/src/common/shell_switches.h"
#include "content/nw/src/nw_notification_manager_toast_win.h"

#include <Psapi.h>
#include <ShObjIdl.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>

#include <intsafe.h>
#include <strsafe.h>
#include <wrl\client.h>
#include <wrl\implements.h>

using namespace Microsoft::WRL;
using namespace Windows::Foundation;

namespace nw {

class StringReferenceWrapper
{
public:

  // Constructor which takes an existing string buffer and its length as the parameters.
  // It fills an HSTRING_HEADER struct with the parameter.      
  // Warning: The caller must ensure the lifetime of the buffer outlives this      
  // object as it does not make a copy of the wide string memory.      

  static bool isSupported()
  {
    static char cachedRes = -1;
    if (cachedRes > -1) return cachedRes;
    cachedRes = ::LoadLibrary(L"API-MS-WIN-CORE-WINRT-STRING-L1-1-0.DLL") != 0;
    return cachedRes;
  }

  StringReferenceWrapper(_In_reads_(length) PCWSTR stringRef, _In_ UINT32 length) throw()
  {
    HRESULT hr = WindowsCreateStringReference(stringRef, length, &_header, &_hstring);

    if (FAILED(hr))
    {
      RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
    }
  }

  ~StringReferenceWrapper()
  {
    WindowsDeleteString(_hstring);
  }

  template <size_t N>
  StringReferenceWrapper(_In_reads_(N) wchar_t const (&stringRef)[N]) throw()
  {
    UINT32 length = N - 1;
    HRESULT hr = WindowsCreateStringReference(stringRef, length, &_header, &_hstring);

    if (FAILED(hr))
    {
      RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
    }
  }

  template <size_t _>
  StringReferenceWrapper(_In_reads_(_) wchar_t(&stringRef)[_]) throw()
  {
    UINT32 length;
    HRESULT hr = SizeTToUInt32(wcslen(stringRef), &length);

    if (FAILED(hr))
    {
      RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
    }

    WindowsCreateStringReference(stringRef, length, &_header, &_hstring);
  }

  HSTRING Get() const throw()
  {
    return _hstring;
  }


private:
  HSTRING             _hstring;
  HSTRING_HEADER      _header;
};

typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ::IInspectable *> DesktopToastActivatedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastDismissedEventArgs *> DesktopToastDismissedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification *, ABI::Windows::UI::Notifications::ToastFailedEventArgs *> DesktopToastFailedEventHandler;

class ToastEventHandler :
  public Microsoft::WRL::Implements<DesktopToastActivatedEventHandler, DesktopToastDismissedEventHandler, DesktopToastFailedEventHandler>
{
public:
  ToastEventHandler::ToastEventHandler(const int render_process_id, const int render_frame_id, const int notification_id);
  ~ToastEventHandler();

  // DesktopToastActivatedEventHandler 
  IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ IInspectable* args);

  // DesktopToastDismissedEventHandler
  IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs *e);

  // DesktopToastFailedEventHandler
  IFACEMETHODIMP Invoke(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastFailedEventArgs *e);

  // IUnknown
  IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_ref); }

  IFACEMETHODIMP_(ULONG) Release() {
    ULONG l = InterlockedDecrement(&_ref);
    if (l == 0)
      delete this;
    return l;
  }

  IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _COM_Outptr_ void **ppv) {
    if (IsEqualIID(riid, IID_IUnknown))
      *ppv = static_cast<IUnknown*>(static_cast<DesktopToastActivatedEventHandler*>(this));
    else if (IsEqualIID(riid, __uuidof(DesktopToastActivatedEventHandler)))
      *ppv = static_cast<DesktopToastActivatedEventHandler*>(this);
    else if (IsEqualIID(riid, __uuidof(DesktopToastDismissedEventHandler)))
      *ppv = static_cast<DesktopToastDismissedEventHandler*>(this);
    else if (IsEqualIID(riid, __uuidof(DesktopToastFailedEventHandler)))
      *ppv = static_cast<DesktopToastFailedEventHandler*>(this);
    else *ppv = nullptr;

    if (*ppv) {
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();
      return S_OK;
    }

    return E_NOINTERFACE;
  }

private:
  ULONG _ref;
  const int _render_process_id, _render_frame_id, _notification_id;
};

// ============= ToastEventHandler Implementation =============

ToastEventHandler::ToastEventHandler(const int render_process_id, const int render_frame_id, const int notification_id) : 
_ref(0), _render_process_id(render_process_id), _render_frame_id(render_frame_id), _notification_id(notification_id)
{

}

ToastEventHandler::~ToastEventHandler()
{

}

// DesktopToastActivatedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification* /* sender */, _In_ IInspectable* /* args */)
{
  BOOL succeeded = nw::NotificationManager::getSingleton()->DesktopNotificationPostClick(_render_process_id, _render_frame_id, _notification_id);
  return succeeded ? S_OK : E_FAIL;
}

// DesktopToastDismissedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification* /* sender */, _In_ IToastDismissedEventArgs* e)
{
  ToastDismissalReason tdr;
  HRESULT hr = e->get_Reason(&tdr);
  if (SUCCEEDED(hr))
  {
    BOOL succeeded = nw::NotificationManager::getSingleton()->DesktopNotificationPostClose(_render_process_id, _render_frame_id, _notification_id, tdr == ToastDismissalReason_UserCanceled);
    hr = succeeded ? S_OK : E_FAIL;
  }
  return hr;
}

// DesktopToastFailedEventHandler
IFACEMETHODIMP ToastEventHandler::Invoke(_In_ IToastNotification* /* sender */, _In_ IToastFailedEventArgs* /* e */)
{
  BOOL succeeded = nw::NotificationManager::getSingleton()->DesktopNotificationPostError(_render_process_id, _render_frame_id, _notification_id, L"The toast encountered an error.");
  return succeeded ? S_OK : E_FAIL;
}

// ============= NotificationManagerToastWin Implementation =============

HRESULT NotificationManagerToastWin::SetNodeValueString(_In_ HSTRING inputString, _In_ IXmlNode *node, _In_ IXmlDocument *xml)
{
  ComPtr<IXmlText> inputText;

  HRESULT hr = xml->CreateTextNode(inputString, &inputText);
  if (SUCCEEDED(hr))
  {
    ComPtr<IXmlNode> inputTextNode;

    hr = inputText.As(&inputTextNode);
    if (SUCCEEDED(hr))
    {
      ComPtr<IXmlNode> pAppendedChild;
      hr = node->AppendChild(inputTextNode.Get(), &pAppendedChild);
    }
  }

  return hr;
}

HRESULT NotificationManagerToastWin::SetTextValues(_In_reads_(textValuesCount) const wchar_t **textValues, _In_ UINT32 textValuesCount, 
  _In_reads_(textValuesCount) UINT32 *textValuesLengths, _In_ IXmlDocument *toastXml)
{
  HRESULT hr = textValues != nullptr && textValuesCount > 0 ? S_OK : E_INVALIDARG;
  if (SUCCEEDED(hr))
  {
    ComPtr<IXmlNodeList> nodeList;
    hr = toastXml->GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList);
    if (SUCCEEDED(hr))
    {
      UINT32 nodeListLength;
      hr = nodeList->get_Length(&nodeListLength);
      if (SUCCEEDED(hr))
      {
        hr = textValuesCount <= nodeListLength ? S_OK : E_INVALIDARG;
        if (SUCCEEDED(hr))
        {
          for (UINT32 i = 0; i < textValuesCount; i++)
          {
            ComPtr<IXmlNode> textNode;
            hr = nodeList->Item(i, &textNode);
            if (SUCCEEDED(hr))
            {
              hr = SetNodeValueString(StringReferenceWrapper(textValues[i], textValuesLengths[i]).Get(), textNode.Get(), toastXml);
            }
          }
        }
      }
    }
  }
  return hr;
}

HRESULT NotificationManagerToastWin::SetImageSrc(_In_z_ const wchar_t *imagePath, _In_ IXmlDocument *toastXml)
{
  wchar_t imageSrc[MAX_PATH] = L"";
  HRESULT hr = StringCchCat(imageSrc, ARRAYSIZE(imageSrc), imagePath);
  if (SUCCEEDED(hr))
  {
    ComPtr<IXmlNodeList> nodeList;
    hr = toastXml->GetElementsByTagName(StringReferenceWrapper(L"image").Get(), &nodeList);
    if (SUCCEEDED(hr))
    {
      ComPtr<IXmlNode> imageNode;
      hr = nodeList->Item(0, &imageNode);
      if (SUCCEEDED(hr))
      {
        ComPtr<IXmlNamedNodeMap> attributes;

        hr = imageNode->get_Attributes(&attributes);
        if (SUCCEEDED(hr))
        {
          ComPtr<IXmlNode> srcAttribute;

          hr = attributes->GetNamedItem(StringReferenceWrapper(L"src").Get(), &srcAttribute);
          if (SUCCEEDED(hr))
          {
            hr = SetNodeValueString(StringReferenceWrapper(imageSrc).Get(), srcAttribute.Get(), toastXml);
          }
        }
      }
    }
  }
  return hr;
}

HRESULT NotificationManagerToastWin::CreateToastXml(_In_ IToastNotificationManagerStatics *toastManager, 
  const content::ShowDesktopNotificationHostMsgParams& params, _Outptr_ IXmlDocument** inputXml)
{
  const bool bImage = params.icon_url.is_valid() && params.icon_url.spec().find("file:///") == 0;
  // Retrieve the template XML
  HRESULT hr = toastManager->GetTemplateContent(bImage ? ToastTemplateType_ToastImageAndText03 : ToastTemplateType_ToastText03, inputXml);
  if (SUCCEEDED(hr))
  {
    if (SUCCEEDED(hr))
    {
      hr = bImage ? SetImageSrc(base::UTF8ToWide(params.icon_url.spec()).c_str(), *inputXml) : S_OK;
      if (SUCCEEDED(hr))
      {
        const wchar_t* textValues[] = {
          params.title.c_str(),
          params.body.c_str()
        };

        UINT32 textLengths[] = { params.title.length(), params.body.length() };

        hr = SetTextValues(textValues, 2, textLengths, *inputXml);
      }
    }
  }
  return hr;
}

HRESULT NotificationManagerToastWin::CreateToast(_In_ IToastNotificationManagerStatics *toastManager, _In_ IXmlDocument *xml, 
  const int render_process_id, const int render_frame_id, const int notification_id)
{
  base::string16 appID;
  if (content::Shell::GetPackage()->root()->GetString("app-id", &appID) == false)
    content::Shell::GetPackage()->root()->GetString(switches::kmName, &appID);

  ComPtr<IToastNotifier> notifier;
  HRESULT hr = toastManager->CreateToastNotifierWithId(StringReferenceWrapper(appID.c_str(), appID.length()).Get(), &notifier);
  if (SUCCEEDED(hr))
  {
    ComPtr<IToastNotificationFactory> factory;
    hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), &factory);
    if (SUCCEEDED(hr))
    {
      ComPtr<IToastNotification> toast;
      hr = factory->CreateToastNotification(xml, &toast);
      if (SUCCEEDED(hr))
      {
        // Register the event handlers
        EventRegistrationToken activatedToken, dismissedToken, failedToken;
        ComPtr<ToastEventHandler> eventHandler = new ToastEventHandler(render_process_id, render_frame_id, notification_id);

        hr = toast->add_Activated(eventHandler.Get(), &activatedToken);
        if (SUCCEEDED(hr))
        {
          hr = toast->add_Dismissed(eventHandler.Get(), &dismissedToken);
          if (SUCCEEDED(hr))
          {
            hr = toast->add_Failed(eventHandler.Get(), &failedToken);
            if (SUCCEEDED(hr))
            {
              hr = notifier->Show(toast.Get());
            }
          }
        }
      }
    }
  }
  return hr;
}

bool NotificationManagerToastWin::IsSupported() {
  static char cachedRes = -1;
  
  if (cachedRes > -1) return cachedRes;
  cachedRes = 0;
  
  if (StringReferenceWrapper::isSupported()) {
    ComPtr<IToastNotificationManagerStatics> toastStatics;
    HRESULT hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &toastStatics);
    cachedRes = SUCCEEDED(hr);
  }
  
  return cachedRes;
}

NotificationManagerToastWin::NotificationManagerToastWin() {
}

NotificationManagerToastWin::~NotificationManagerToastWin() {

}

bool NotificationManagerToastWin::AddDesktopNotification(const content::ShowDesktopNotificationHostMsgParams& params,
  const int render_process_id, const int render_frame_id, const int notification_id, 
  const bool worker, const std::vector<SkBitmap>* bitmaps) {

  content::RenderViewHost* host = content::RenderFrameHost::FromID(render_process_id, render_frame_id)->GetRenderViewHost();
  if (host == NULL)
    return false;

  ComPtr<IToastNotificationManagerStatics> toastStatics;
  HRESULT hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &toastStatics);
  if (SUCCEEDED(hr))
  {
    ComPtr<IXmlDocument> toastXml;
    hr = CreateToastXml(toastStatics.Get(), params, &toastXml);
    if (SUCCEEDED(hr))
    {
      hr = CreateToast(toastStatics.Get(), toastXml.Get(), render_process_id, render_frame_id, notification_id);
      if (SUCCEEDED(hr))
        DesktopNotificationPostDisplay(render_process_id, render_frame_id, notification_id);
    }
  }
  
  return SUCCEEDED(hr);
}

bool NotificationManagerToastWin::CancelDesktopNotification(int render_process_id, int render_frame_id, int notification_id) {
  return true;
}
} // namespace nw

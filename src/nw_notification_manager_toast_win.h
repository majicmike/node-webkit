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

#ifndef CONTENT_NW_NOTIFICATION_MANAGER_TOAST_WIN_H_
#define CONTENT_NW_NOTIFICATION_MANAGER_TOAST_WIN_H_

#include "content/nw/src/nw_notification_manager.h"
#include <windows.ui.notifications.h>

namespace nw {
  using namespace ABI::Windows::UI::Notifications;
  using namespace ABI::Windows::Data::Xml::Dom;

class NotificationManagerToastWin : public NotificationManager{

  // internal function for AddDesktopNotification
  virtual bool AddDesktopNotification(const content::ShowDesktopNotificationHostMsgParams& params,
    const int render_process_id, const int render_frame_id, const int notification_id, 
    const bool worker, const std::vector<SkBitmap>* bitmaps) OVERRIDE;


  HRESULT CreateToast(_In_ IToastNotificationManagerStatics *toastManager, _In_ IXmlDocument *xml,
    const int render_process_id, const int render_frame_id, const int notification_id);

  // Create the toast XML from a template
  HRESULT CreateToastXml(_In_ IToastNotificationManagerStatics *toastManager,
    const content::ShowDesktopNotificationHostMsgParams& params, _Outptr_ IXmlDocument** inputXml);

  // Set the value of the "src" attribute of the "image" node
  HRESULT SetImageSrc(_In_z_ const wchar_t *imagePath, _In_ IXmlDocument *toastXml);

  // Set the values of each of the text nodes
  HRESULT SetTextValues(_In_reads_(textValuesCount) const wchar_t **textValues, _In_ UINT32 textValuesCount,
    _In_reads_(textValuesCount) UINT32 *textValuesLengths, _In_ IXmlDocument *toastXml);

  HRESULT SetNodeValueString(_In_ HSTRING inputString, _In_ IXmlNode *node, _In_ IXmlDocument *xml);

public:
  static bool IsSupported();
  explicit NotificationManagerToastWin();
  virtual ~NotificationManagerToastWin();
  virtual bool AddDesktopNotification(const content::ShowDesktopNotificationHostMsgParams& params,
    const int render_process_id, const int render_frame_id, const int notification_id,
    const bool worker) OVERRIDE{
    return AddDesktopNotification(params, render_process_id, render_frame_id, notification_id, worker, NULL);
  }
  virtual bool CancelDesktopNotification(int render_process_id, int render_frame_id, int notification_id) OVERRIDE;
};

} // namespace nw

#endif // CONTENT_NW_NOTIFICATION_MANAGER_WIN_H_

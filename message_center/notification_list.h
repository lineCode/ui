// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_NOTIFICATION_LIST_H_
#define UI_MESSAGE_CENTER_NOTIFICATION_LIST_H_

#include <list>
#include <string>

#include "base/string16.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/message_center/message_center_export.h"

namespace message_center {

// A helper class to manage the list of notifications.
class MESSAGE_CENTER_EXPORT NotificationList {
 public:
  struct MESSAGE_CENTER_EXPORT Notification {
    Notification();
    virtual ~Notification();

    std::string id;
    string16 title;
    string16 message;
    string16 display_source;
    std::string extension_id;
    gfx::ImageSkia image;
    bool is_read;  // True if this has been seen in the message center
    bool shown_as_popup;  // True if this has been shown as a popup notification
  };

  typedef std::list<Notification> Notifications;

  class MESSAGE_CENTER_EXPORT Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}

    // Removes notifications
    virtual void SendRemoveNotification(const std::string& id) = 0;
    virtual void SendRemoveAllNotifications() = 0;

    // Disables notifications
    virtual void DisableNotificationByExtension(const std::string& id) = 0;
    virtual void DisableNotificationByUrl(const std::string& id) = 0;

    // Requests the Delegate to the settings dialog.
    virtual void ShowNotificationSettings(const std::string& id) = 0;

    // Called when a notification is clicked on.
    virtual void OnNotificationClicked(const std::string& id) = 0;

    // Returns the list of notifications to display.
    virtual NotificationList* GetNotificationList() = 0;
  };

  explicit NotificationList(Delegate* delegate);
  virtual ~NotificationList();

  // Affects whether or not a message has been "read".
  void SetMessageCenterVisible(bool visible);

  void AddNotification(const std::string& id,
                       const string16& title,
                       const string16& message,
                       const string16& display_source,
                       const std::string& extension_id);

  void UpdateNotificationMessage(const std::string& old_id,
                                 const std::string& new_id,
                                 const string16& title,
                                 const string16& message);

  // Returns true if the notification was removed.
  bool RemoveNotification(const std::string& id);

  void RemoveAllNotifications();

  void SendRemoveNotificationsBySource(const std::string& id);

  void SendRemoveNotificationsByExtension(const std::string& id);

  // Returns true if the notification exists and was updated.
  bool SetNotificationImage(const std::string& id,
                            const gfx::ImageSkia& image);

  bool HasNotification(const std::string& id);

  // Returns false if the first notification has been shown as a popup (which
  // means that all notifications have been shown).
  bool HasPopupNotifications();

  // Modifies |notifications| to contain the |kMaxVisiblePopupNotifications|
  // least recent notifications that have not been shown as a popup.
  void GetPopupNotifications(Notifications* notifications);

  // Marks the popups returned by GetPopupNotifications() as shown.
  void MarkPopupsAsShown();

  const Notifications& notifications() const { return notifications_; }
  size_t unread_count() const { return unread_count_; }

  static const size_t kMaxVisiblePopupNotifications;
  static const size_t kMaxVisibleMessageCenterNotifications;

 private:
  // Iterates through the list and returns the first notification matching |id|
  // (should always be unique).
  Notifications::iterator GetNotification(const std::string& id);

  void EraseNotification(Notifications::iterator iter);

  void PushNotification(Notification& notification);

  // Returns the |kMaxVisiblePopupNotifications| least recent notifications
  // that have not been shown as a popup.
  void GetPopupIterators(Notifications::iterator& first,
                         Notifications::iterator& last);

  Delegate* delegate_;
  Notifications notifications_;
  bool message_center_visible_;
  size_t unread_count_;

  DISALLOW_COPY_AND_ASSIGN(NotificationList);
};

}  // namespace message_center

#endif // UI_MESSAGE_CENTER_NOTIFICATION_LIST_H_

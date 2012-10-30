// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/notification_list.h"

namespace message_center {

const size_t NotificationList::kMaxVisibleMessageCenterNotifications = 100;
const size_t NotificationList::kMaxVisiblePopupNotifications = 5;

NotificationList::Notification::Notification() : is_read(false),
                                                 shown_as_popup(false) {
}

NotificationList::Notification::~Notification() {
}

NotificationList::NotificationList(Delegate* delegate)
    : delegate_(delegate),
      message_center_visible_(false),
      unread_count_(0) {
}

NotificationList::~NotificationList() {
}

void NotificationList::SetMessageCenterVisible(bool visible) {
  if (message_center_visible_ == visible)
    return;
  message_center_visible_ = visible;
  if (!visible) {
    // When the list is hidden, clear the unread count, and mark all
    // notifications as read and shown.
    unread_count_ = 0;
    for (Notifications::iterator iter = notifications_.begin();
         iter != notifications_.end(); ++iter) {
      iter->is_read = true;
      iter->shown_as_popup = true;
    }
  }
}

void NotificationList::AddNotification(const std::string& id,
                                       const string16& title,
                                       const string16& message,
                                       const string16& display_source,
                                       const std::string& extension_id) {
  Notification notification;
  notification.id = id;
  notification.title = title;
  notification.message = message;
  notification.display_source = display_source;
  notification.extension_id = extension_id;
  PushNotification(notification);
}

void NotificationList::UpdateNotificationMessage(const std::string& old_id,
                                                 const std::string& new_id,
                                                 const string16& title,
                                                 const string16& message) {
  Notifications::iterator iter = GetNotification(old_id);
  if (iter == notifications_.end())
    return;
  // Copy and update notification, then move it to the front of the list.
  Notification notification(*iter);
  notification.id = new_id;
  notification.title = title;
  notification.message = message;
  EraseNotification(iter);
  PushNotification(notification);
}

bool NotificationList::RemoveNotification(const std::string& id) {
  Notifications::iterator iter = GetNotification(id);
  if (iter == notifications_.end())
    return false;
  EraseNotification(iter);
  return true;
}

void NotificationList::RemoveAllNotifications() {
  notifications_.clear();
}

void NotificationList::SendRemoveNotificationsBySource(
    const std::string& id) {
  Notifications::iterator source_iter = GetNotification(id);
  if (source_iter == notifications_.end())
    return;
  string16 display_source = source_iter->display_source;
  for (Notifications::iterator loopiter = notifications_.begin();
       loopiter != notifications_.end(); ) {
    Notifications::iterator curiter = loopiter++;
    if (curiter->display_source == display_source)
      delegate_->SendRemoveNotification(curiter->id);
  }
}

void NotificationList::SendRemoveNotificationsByExtension(
    const std::string& id) {
  Notifications::iterator source_iter = GetNotification(id);
  if (source_iter == notifications_.end())
    return;
  std::string extension_id = source_iter->extension_id;
  for (Notifications::iterator loopiter = notifications_.begin();
       loopiter != notifications_.end(); ) {
    Notifications::iterator curiter = loopiter++;
    if (curiter->extension_id == extension_id)
      delegate_->SendRemoveNotification(curiter->id);
  }
}

bool NotificationList::SetNotificationImage(const std::string& id,
                                            const gfx::ImageSkia& image) {
  Notifications::iterator iter = GetNotification(id);
  if (iter == notifications_.end())
    return false;
  iter->image = image;
  return true;
}

bool NotificationList::HasNotification(const std::string& id) {
  return GetNotification(id) != notifications_.end();
}

bool NotificationList::HasPopupNotifications() {
  return !notifications_.empty() && !notifications_.front().shown_as_popup;
}

void NotificationList::GetPopupNotifications(
    NotificationList::Notifications* notifications) {
  Notifications::iterator first, last;
  GetPopupIterators(first, last);
  notifications->clear();
  for (Notifications::iterator iter = first; iter != last; ++iter)
    notifications->push_back(*iter);
}

void NotificationList::MarkPopupsAsShown() {
  Notifications::iterator first, last;
  GetPopupIterators(first, last);
  for (Notifications::iterator iter = first; iter != last; ++iter)
    iter->shown_as_popup = true;
}

NotificationList::Notifications::iterator NotificationList::GetNotification(
    const std::string& id) {
  for (Notifications::iterator iter = notifications_.begin();
       iter != notifications_.end(); ++iter) {
    if (iter->id == id)
      return iter;
  }
  return notifications_.end();
}

void NotificationList::EraseNotification(Notifications::iterator iter) {
  if (!message_center_visible_ && !iter->is_read)
    --unread_count_;
  notifications_.erase(iter);
}

void NotificationList::PushNotification(Notification& notification) {
  // Ensure that notification.id is unique by erasing any existing
  // notification with the same id (shouldn't normally happen).
  Notifications::iterator iter = GetNotification(notification.id);
  if (iter != notifications_.end())
    EraseNotification(iter);
  // Add the notification to the front (top) of the list and mark it
  // unread and unshown.
  if (!message_center_visible_) {
    ++unread_count_;
    notification.is_read = false;
    notification.shown_as_popup = false;
  }
  notifications_.push_front(notification);
}

void NotificationList::GetPopupIterators(Notifications::iterator& first,
                                         Notifications::iterator& last) {
  size_t popup_count = 0;
  first = notifications_.begin();
  last = first;
  while (last != notifications_.end()) {
    if (last->shown_as_popup)
      break;
    ++last;
    if (popup_count < kMaxVisiblePopupNotifications)
      ++popup_count;
    else
      ++first;
  }
}

}  // namespace message_center

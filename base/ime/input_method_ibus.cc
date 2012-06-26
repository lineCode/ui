// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/input_method_ibus.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef FocusIn
#undef FocusOut

#include <algorithm>
#include <cstring>
#include <set>
#include <vector>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/i18n/char_iterator.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/third_party/icu/icu_utf.h"
#include "base/utf_string_conversions.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/ibus/ibus_input_context_client.h"
#include "chromeos/dbus/ibus/ibus_text.h"
#include "ui/base/events.h"
#include "ui/base/ime/ibus_client_impl.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/keycodes/keyboard_code_conversion.h"
#include "ui/base/keycodes/keyboard_code_conversion_x.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "ui/gfx/rect.h"

namespace {

const int kIBusReleaseMask = 1 << 30;

XKeyEvent* GetKeyEvent(XEvent* event) {
  DCHECK(event && (event->type == KeyPress || event->type == KeyRelease));
  return &event->xkey;
}

// Converts X (and ibus) flags to event flags.
int EventFlagsFromXFlags(unsigned int flags) {
  return (flags & LockMask ? ui::EF_CAPS_LOCK_DOWN : 0) |
      (flags & ControlMask ? ui::EF_CONTROL_DOWN : 0) |
      (flags & ShiftMask ? ui::EF_SHIFT_DOWN : 0) |
      (flags & Mod1Mask ? ui::EF_ALT_DOWN : 0) |
      (flags & Button1Mask ? ui::EF_LEFT_MOUSE_BUTTON : 0) |
      (flags & Button2Mask ? ui::EF_MIDDLE_MOUSE_BUTTON : 0) |
      (flags & Button3Mask ? ui::EF_RIGHT_MOUSE_BUTTON : 0);
}

// Converts X flags to ibus key state flags.
uint32 IBusStateFromXFlags(unsigned int flags) {
  return (flags & (LockMask | ControlMask | ShiftMask | Mod1Mask |
                   Button1Mask | Button2Mask | Button3Mask));
}

void IBusKeyEventFromNativeKeyEvent(const base::NativeEvent& native_event,
                                    uint32* ibus_keyval,
                                    uint32* ibus_keycode,
                                    uint32* ibus_state) {
  DCHECK(native_event);  // A fabricated event is not supported here.
  XKeyEvent* x_key = GetKeyEvent(native_event);

  // Yes, ibus uses X11 keysym. We cannot use XLookupKeysym(), which doesn't
  // translate Shift and CapsLock states.
  KeySym keysym = NoSymbol;
  ::XLookupString(x_key, NULL, 0, &keysym, NULL);
  *ibus_keyval = keysym;
  *ibus_keycode = x_key->keycode;
  *ibus_state = IBusStateFromXFlags(x_key->state);
  if (native_event->type == KeyRelease)
    *ibus_state |= kIBusReleaseMask;
}

}  // namespace

namespace ui {

// InputMethodIBus::PendingKeyEventImpl implementation ------------------------
class InputMethodIBus::PendingKeyEventImpl
    : public internal::IBusClient::PendingKeyEvent {
 public:
  PendingKeyEventImpl(InputMethodIBus* input_method,
                      const base::NativeEvent& native_event,
                      uint32 ibus_keyval);
  virtual ~PendingKeyEventImpl();

  // internal::IBusClient::PendingKeyEvent overrides:
  virtual void ProcessPostIME(bool handled) OVERRIDE;

  // Abandon this pending key event. Its result will just be discarded.
  void Abandon() { input_method_ = NULL; }

  InputMethodIBus* input_method() const { return input_method_; }

 private:
  InputMethodIBus* input_method_;

  // TODO(yusukes): To support a fabricated key event (which is typically from
  // a virtual keyboard), we might have to copy event type, event flags, key
  // code, 'character_', and 'unmodified_character_'. See views::InputMethodIBus
  // for details.

  // corresponding XEvent data of a key event. It's a plain struct so we can do
  // bitwise copy.
  XKeyEvent x_event_;

  const uint32 ibus_keyval_;

  DISALLOW_COPY_AND_ASSIGN(PendingKeyEventImpl);
};

InputMethodIBus::PendingKeyEventImpl::PendingKeyEventImpl(
    InputMethodIBus* input_method,
    const base::NativeEvent& native_event,
    uint32 ibus_keyval)
    : input_method_(input_method),
      ibus_keyval_(ibus_keyval) {
  DCHECK(input_method_);

  // TODO(yusukes): Support non-native event (from e.g. a virtual keyboard).
  DCHECK(native_event);
  x_event_ = *GetKeyEvent(native_event);
}

InputMethodIBus::PendingKeyEventImpl::~PendingKeyEventImpl() {
  if (input_method_)
    input_method_->FinishPendingKeyEvent(this);
}

void InputMethodIBus::PendingKeyEventImpl::ProcessPostIME(bool handled) {
  if (!input_method_)
    return;

  if (x_event_.type == KeyPress || x_event_.type == KeyRelease) {
    input_method_->ProcessKeyEventPostIME(reinterpret_cast<XEvent*>(&x_event_),
                                          ibus_keyval_,
                                          handled);
    return;
  }

  // TODO(yusukes): Support non-native event (from e.g. a virtual keyboard).
  // See views::InputMethodIBus for details. Never forget to set 'character_'
  // and 'unmodified_character_' to support i18n VKs like a French VK!
}

// InputMethodIBus::PendingCreateICRequestImpl implementation -----------------
class InputMethodIBus::PendingCreateICRequestImpl
    : public internal::IBusClient::PendingCreateICRequest {
 public:
  PendingCreateICRequestImpl(InputMethodIBus* input_method,
                             internal::IBusClient* ibus_client,
                             PendingCreateICRequestImpl** request_ptr);
  virtual ~PendingCreateICRequestImpl();

  // internal::IBusClient::PendingCreateICRequest overrides:
  virtual void InitOrAbandonInputContext() OVERRIDE;
  virtual void OnCreateInputContextFailed() OVERRIDE;

  // Abandon this pending key event. Its result will just be discarded.
  void Abandon() {
    input_method_ = NULL;
    request_ptr_ = NULL;
    // Do not reset |ibus_client_| here.
  }

 private:
  InputMethodIBus* input_method_;
  internal::IBusClient* ibus_client_;
  PendingCreateICRequestImpl** request_ptr_;

  DISALLOW_COPY_AND_ASSIGN(PendingCreateICRequestImpl);
};

InputMethodIBus::PendingCreateICRequestImpl::PendingCreateICRequestImpl(
    InputMethodIBus* input_method,
    internal::IBusClient* ibus_client,
    PendingCreateICRequestImpl** request_ptr)
    : input_method_(input_method),
      ibus_client_(ibus_client),
      request_ptr_(request_ptr) {
}

InputMethodIBus::PendingCreateICRequestImpl::~PendingCreateICRequestImpl() {
  if (request_ptr_) {
    DCHECK_EQ(*request_ptr_, this);
    *request_ptr_ = NULL;
  }
}

void InputMethodIBus::PendingCreateICRequestImpl::OnCreateInputContextFailed() {
  // TODO(nona): If the connection between Chrome and ibus-daemon terminates
  // for some reason, the create ic request will fail. We might want to call
  // ibus_client_->CreateContext() again after some delay.
}

void InputMethodIBus::PendingCreateICRequestImpl::InitOrAbandonInputContext() {
  if (input_method_) {
    DCHECK(ibus_client_->IsContextReady());
    input_method_->SetUpSignalHandlers();
  } else {
    ibus_client_->DestroyProxy();
    DCHECK(!ibus_client_->IsContextReady());
  }
}

// InputMethodIBus implementation -----------------------------------------
InputMethodIBus::InputMethodIBus(
    internal::InputMethodDelegate* delegate)
    :
      ibus_client_(new internal::IBusClientImpl),
      pending_create_ic_request_(NULL),
      context_focused_(false),
      composing_text_(false),
      composition_changed_(false),
      suppress_next_result_(false),
      weak_ptr_factory_(this) {
  SetDelegate(delegate);
}

InputMethodIBus::~InputMethodIBus() {
  AbandonAllPendingKeyEvents();
  DestroyContext();
}

void InputMethodIBus::set_ibus_client(
    scoped_ptr<internal::IBusClient> new_client) {
  ibus_client_.swap(new_client);
}

internal::IBusClient* InputMethodIBus::ibus_client() const {
  return ibus_client_.get();
}

void InputMethodIBus::OnFocus() {
  InputMethodBase::OnFocus();
  UpdateContextFocusState();
}

void InputMethodIBus::OnBlur() {
  ConfirmCompositionText();
  InputMethodBase::OnBlur();
  UpdateContextFocusState();
}

void InputMethodIBus::Init(bool focused) {
  // Initializes the connection to ibus daemon. It may happen asynchronously,
  // and as soon as the connection is established, the |context_| will be
  // created automatically.

  // Create the input context if the connection is already established.
  if (ibus_client_->IsConnected())
    CreateContext();

  InputMethodBase::Init(focused);
}

// static
void InputMethodIBus::ProcessKeyEventDone(
    PendingKeyEventImpl* pending_key_event, bool is_handled) {
  DCHECK(pending_key_event);
  pending_key_event->ProcessPostIME(is_handled);
  delete pending_key_event;
}

void InputMethodIBus::DispatchKeyEvent(const base::NativeEvent& native_event) {
  DCHECK(native_event && (native_event->type == KeyPress ||
                          native_event->type == KeyRelease));
  DCHECK(system_toplevel_window_focused());

  uint32 ibus_keyval = 0;
  uint32 ibus_keycode = 0;
  uint32 ibus_state = 0;
  IBusKeyEventFromNativeKeyEvent(
      native_event, &ibus_keyval, &ibus_keycode, &ibus_state);

  // If |context_| is not usable, then we can only dispatch the key event as is.
  // We also dispatch the key event directly if the current text input type is
  // TEXT_INPUT_TYPE_PASSWORD, to bypass the input method.
  // Note: We need to send the key event to ibus even if the |context_| is not
  // enabled, so that ibus can have a chance to enable the |context_|.
  if (!context_focused_ ||
      GetTextInputType() == TEXT_INPUT_TYPE_PASSWORD ||
      ibus_client_->GetInputMethodType() ==
      internal::IBusClient::INPUT_METHOD_XKB_LAYOUT) {
    if (native_event->type == KeyPress)
      ProcessUnfilteredKeyPressEvent(native_event, ibus_keyval);
    else
      DispatchKeyEventPostIME(native_event);
    return;
  }

  PendingKeyEventImpl* pending_key =
      new PendingKeyEventImpl(this, native_event, ibus_keyval);
  pending_key_events_.insert(pending_key);

  ibus_client_->SendKeyEvent(ibus_keyval,
                             ibus_keycode,
                             ibus_state,
                             base::Bind(&InputMethodIBus::ProcessKeyEventDone,
                                        base::Unretained(pending_key)));

  // We don't want to suppress the result generated by this key event, but it
  // may cause problem. See comment in ResetContext() method.
  suppress_next_result_ = false;
}

void InputMethodIBus::OnTextInputTypeChanged(const TextInputClient* client) {
  if (ibus_client_->IsContextReady() && IsTextInputClientFocused(client)) {
    ResetContext();
    UpdateContextFocusState();
  }
  InputMethodBase::OnTextInputTypeChanged(client);
}

void InputMethodIBus::OnCaretBoundsChanged(const TextInputClient* client) {
  if (!context_focused_ || !IsTextInputClientFocused(client))
    return;

  // The current text input type should not be NONE if |context_| is focused.
  DCHECK(!IsTextInputTypeNone());
  const gfx::Rect rect = GetTextInputClient()->GetCaretBounds();

  gfx::Rect composition_head;
  if (!GetTextInputClient()->GetCompositionCharacterBounds(0,
                                                           &composition_head)) {
    composition_head = rect;
  }

  // This function runs asynchronously.
  ibus_client_->SetCursorLocation(rect, composition_head);
}

void InputMethodIBus::CancelComposition(const TextInputClient* client) {
  if (context_focused_ && IsTextInputClientFocused(client))
    ResetContext();
}

std::string InputMethodIBus::GetInputLocale() {
  // Not supported.
  return "";
}

base::i18n::TextDirection InputMethodIBus::GetInputTextDirection() {
  // Not supported.
  return base::i18n::UNKNOWN_DIRECTION;
}

bool InputMethodIBus::IsActive() {
  return true;
}

void InputMethodIBus::OnWillChangeFocusedClient(TextInputClient* focused_before,
                                                TextInputClient* focused) {
  ConfirmCompositionText();
}

void InputMethodIBus::OnDidChangeFocusedClient(TextInputClient* focused_before,
                                               TextInputClient* focused) {
  // Force to update the input type since client's TextInputStateChanged()
  // function might not be called if text input types before the client loses
  // focus and after it acquires focus again are the same.
  OnTextInputTypeChanged(focused);

  UpdateContextFocusState();
  // Force to update caret bounds, in case the client thinks that the caret
  // bounds has not changed.
  OnCaretBoundsChanged(focused);
}

void InputMethodIBus::CreateContext() {
  DCHECK(ibus_client_->IsConnected());
  DCHECK(!pending_create_ic_request_);

  // Creates the input context asynchronously.
  pending_create_ic_request_ = new PendingCreateICRequestImpl(
      this, ibus_client_.get(), &pending_create_ic_request_);
  ibus_client_->CreateContext(pending_create_ic_request_);
}

void InputMethodIBus::SetUpSignalHandlers() {
  DCHECK(ibus_client_->IsContextReady());

  // connect input context signals
  chromeos::IBusInputContextClient* input_context_client =
      chromeos::DBusThreadManager::Get()->GetIBusInputContextClient();
  input_context_client->SetCommitTextHandler(
      base::Bind(&InputMethodIBus::OnCommitText,
                 weak_ptr_factory_.GetWeakPtr()));

  input_context_client->SetForwardKeyEventHandler(
      base::Bind(&InputMethodIBus::OnForwardKeyEvent,
                 weak_ptr_factory_.GetWeakPtr()));

  input_context_client->SetUpdatePreeditTextHandler(
      base::Bind(&InputMethodIBus::OnUpdatePreeditText,
                 weak_ptr_factory_.GetWeakPtr()));

  input_context_client->SetShowPreeditTextHandler(
      base::Bind(&InputMethodIBus::OnShowPreeditText,
                 weak_ptr_factory_.GetWeakPtr()));

  input_context_client->SetHidePreeditTextHandler(
      base::Bind(&InputMethodIBus::OnHidePreeditText,
                 weak_ptr_factory_.GetWeakPtr()));

  ibus_client_->SetCapabilities(internal::IBusClient::INLINE_COMPOSITION);

  UpdateContextFocusState();
  // Since ibus-daemon is launched in an on-demand basis on Chrome OS, RWHVA (or
  // equivalents) might call OnCaretBoundsChanged() before the daemon starts. To
  // save the case, call OnCaretBoundsChanged() here.
  OnCaretBoundsChanged(GetTextInputClient());
  OnInputMethodChanged();
}

void InputMethodIBus::DestroyContext() {
  if (pending_create_ic_request_) {
    DCHECK(!ibus_client_->IsContextReady());
    // |pending_create_ic_request_| will be deleted in CreateInputContextDone().
    pending_create_ic_request_->Abandon();
    pending_create_ic_request_ = NULL;
  } else if (chromeos::DBusThreadManager::Get()->GetIBusInputContextClient()
                ->IsObjectProxyReady()) {
    // We can't use IsContextReady here because we want to destroy object proxy
    // regardless of connection. The IsContextReady contains connection check.
    ResetInputContext();
    DCHECK(!ibus_client_->IsContextReady());
  }
}

void InputMethodIBus::ConfirmCompositionText() {
  TextInputClient* client = GetTextInputClient();
  if (client && client->HasCompositionText())
    client->ConfirmCompositionText();

  ResetContext();
}

void InputMethodIBus::ResetContext() {
  if (!context_focused_ || !GetTextInputClient())
    return;

  DCHECK(system_toplevel_window_focused());

  // Because ibus runs in asynchronous mode, the input method may still send us
  // results after sending out the reset request, so we use a flag to discard
  // all results generated by previous key events. But because ibus does not
  // have a mechanism to identify each key event and corresponding results, this
  // approach will not work for some corner cases. For example if the user types
  // very fast, then the next key event may come in before the |context_| is
  // really reset. Then we actually cannot know whether or not the next
  // result should be discard.
  suppress_next_result_ = true;

  composition_.Clear();
  result_text_.clear();
  composing_text_ = false;
  composition_changed_ = false;

  // We need to abandon all pending key events, but as above comment says, there
  // is no reliable way to abandon all results generated by these abandoned key
  // events.
  AbandonAllPendingKeyEvents();

  // This function runs asynchronously.
  // Note: some input method engines may not support reset method, such as
  // ibus-anthy. But as we control all input method engines by ourselves, we can
  // make sure that all of the engines we are using support it correctly.
  ibus_client_->Reset();

  character_composer_.Reset();
}

void InputMethodIBus::UpdateContextFocusState() {
  if (!ibus_client_->IsContextReady()) {
    context_focused_ = false;
    return;
  }

  const bool old_context_focused = context_focused_;
  // Use switch here in case we are going to add more text input types.
  switch (GetTextInputType()) {
    case TEXT_INPUT_TYPE_NONE:
    case TEXT_INPUT_TYPE_PASSWORD:
      context_focused_ = false;
      break;
    default:
      context_focused_ = true;
      break;
  }

  // We only focus in |context_| when the focus is in a normal textfield.
  // ibus_input_context_focus_{in|out}() run asynchronously.
  if (old_context_focused && !context_focused_)
    ibus_client_->FocusOut();
  else if (!old_context_focused && context_focused_)
    ibus_client_->FocusIn();

  if (context_focused_) {
    internal::IBusClient::InlineCompositionCapability capability =
        CanComposeInline() ? internal::IBusClient::INLINE_COMPOSITION
                           : internal::IBusClient::OFF_THE_SPOT_COMPOSITION;
    ibus_client_->SetCapabilities(capability);
  }
}

void InputMethodIBus::ProcessKeyEventPostIME(
    const base::NativeEvent& native_event,
    uint32 ibus_keyval,
    bool handled) {
  TextInputClient* client = GetTextInputClient();

  if (!client) {
    // As ibus works asynchronously, there is a chance that the focused client
    // loses focus before this method gets called.
    DispatchKeyEventPostIME(native_event);
    return;
  }

  if (native_event->type == KeyPress && handled)
    ProcessFilteredKeyPressEvent(native_event);

  // In case the focus was changed by the key event. The |context_| should have
  // been reset when the focused window changed.
  if (client != GetTextInputClient())
    return;

  if (HasInputMethodResult())
    ProcessInputMethodResult(native_event, handled);

  // In case the focus was changed when sending input method results to the
  // focused window.
  if (client != GetTextInputClient())
    return;

  if (native_event->type == KeyPress && !handled)
    ProcessUnfilteredKeyPressEvent(native_event, ibus_keyval);
  else if (native_event->type == KeyRelease)
    DispatchKeyEventPostIME(native_event);
}

void InputMethodIBus::ProcessFilteredKeyPressEvent(
    const base::NativeEvent& native_event) {
  if (NeedInsertChar())
    DispatchKeyEventPostIME(native_event);
  else
    DispatchFabricatedKeyEventPostIME(
        ET_KEY_PRESSED,
        VKEY_PROCESSKEY,
        EventFlagsFromXFlags(GetKeyEvent(native_event)->state));
}

void InputMethodIBus::ProcessUnfilteredKeyPressEvent(
    const base::NativeEvent& native_event,
    uint32 ibus_keyval) {
  // For a fabricated event, ProcessUnfilteredFabricatedKeyPressEvent should be
  // called instead.
  DCHECK(native_event);

  TextInputClient* client = GetTextInputClient();
  DispatchKeyEventPostIME(native_event);

  // We shouldn't dispatch the character anymore if the key event dispatch
  // caused focus change. For example, in the following scenario,
  // 1. visit a web page which has a <textarea>.
  // 2. click Omnibox.
  // 3. enable Korean IME, press A, then press Tab to move the focus to the web
  //    page.
  // We should return here not to send the Tab key event to RWHV.
  if (client != GetTextInputClient())
    return;

  const uint32 state =
      EventFlagsFromXFlags(GetKeyEvent(native_event)->state);

  // Process compose and dead keys
  if (character_composer_.FilterKeyPress(ibus_keyval, state)) {
    string16 composed = character_composer_.composed_character();
    if (!composed.empty()) {
      client = GetTextInputClient();
      if (client) {
        // TODO(hashimoto): Send correct DOM event for characters which cannot
        // be inserted with InsertChar(). Without sending those events, composed
        // character will not be shown on docs.google.com. crbug.com/133269
        if (composed.size() == 1)
          client->InsertChar(composed[0], state);
        else
          client->InsertText(composed);
      }
    }
    return;
  }

  // If a key event was not filtered by |context_| and |character_composer_|,
  // then it means the key event didn't generate any result text. So we need
  // to send corresponding character to the focused text input client.
  client = GetTextInputClient();

  uint16 ch = 0;
  if (!(state & ui::EF_CONTROL_DOWN))
    ch = ui::GetCharacterFromXEvent(native_event);
  if (!ch) {
    ch = ui::GetCharacterFromKeyCode(
        ui::KeyboardCodeFromNative(native_event), state);
  }

  if (client && ch)
    client->InsertChar(ch, state);
}

void InputMethodIBus::ProcessUnfilteredFabricatedKeyPressEvent(
    EventType type,
    KeyboardCode key_code,
    int flags,
    uint32 ibus_keyval) {
  TextInputClient* client = GetTextInputClient();
  DispatchFabricatedKeyEventPostIME(type, key_code, flags);

  if (client != GetTextInputClient())
    return;

  if (character_composer_.FilterKeyPress(ibus_keyval, flags)) {
    string16 composed = character_composer_.composed_character();
    if (!composed.empty()) {
      client = GetTextInputClient();
      if (client) {
        // TODO(hashimoto): Send correct DOM event for characters which cannot
        // be inserted with InsertChar(). Without sending those events, composed
        // character will not be shown on docs.google.com. crbug.com/133269
        if (composed.size() == 1)
          client->InsertChar(composed[0], flags);
        else
          client->InsertText(composed);
      }
    }
    return;
  }

  client = GetTextInputClient();
  const uint16 ch = ui::GetCharacterFromKeyCode(key_code, flags);
  if (client && ch)
    client->InsertChar(ch, flags);
}

void InputMethodIBus::ProcessInputMethodResult(
    const base::NativeEvent& native_event,
    bool handled) {
  TextInputClient* client = GetTextInputClient();
  DCHECK(client);

  if (result_text_.length()) {
    if (handled && NeedInsertChar()) {
      const uint32 state =
          EventFlagsFromXFlags(GetKeyEvent(native_event)->state);
      for (string16::const_iterator i = result_text_.begin();
           i != result_text_.end(); ++i) {
        client->InsertChar(*i, state);
      }
    } else {
      client->InsertText(result_text_);
      composing_text_ = false;
    }
  }

  if (composition_changed_ && !IsTextInputTypeNone()) {
    if (composition_.text.length()) {
      composing_text_ = true;
      client->SetCompositionText(composition_);
    } else if (result_text_.empty()) {
      client->ClearCompositionText();
    }
  }

  // We should not clear composition text here, as it may belong to the next
  // composition session.
  result_text_.clear();
  composition_changed_ = false;
}

bool InputMethodIBus::NeedInsertChar() const {
  return GetTextInputClient() &&
      (IsTextInputTypeNone() ||
       (!composing_text_ && result_text_.length() == 1));
}

bool InputMethodIBus::HasInputMethodResult() const {
  return result_text_.length() || composition_changed_;
}

void InputMethodIBus::SendFakeProcessKeyEvent(bool pressed) const {
  DispatchFabricatedKeyEventPostIME(pressed ? ET_KEY_PRESSED : ET_KEY_RELEASED,
                                    VKEY_PROCESSKEY,
                                    0);
}

void InputMethodIBus::FinishPendingKeyEvent(PendingKeyEventImpl* pending_key) {
  DCHECK(pending_key_events_.count(pending_key));

  // |pending_key| will be deleted in ProcessKeyEventDone().
  pending_key_events_.erase(pending_key);
}

void InputMethodIBus::AbandonAllPendingKeyEvents() {
  std::set<PendingKeyEventImpl*>::iterator i;
  for (i = pending_key_events_.begin(); i != pending_key_events_.end(); ++i) {
    // The object will be deleted in ProcessKeyEventDone().
    (*i)->Abandon();
  }
  pending_key_events_.clear();
}

void InputMethodIBus::OnCommitText(const chromeos::ibus::IBusText& text) {
  if (suppress_next_result_ || text.text().empty())
    return;

  // We need to receive input method result even if the text input type is
  // TEXT_INPUT_TYPE_NONE, to make sure we can always send correct
  // character for each key event to the focused text input client.
  if (!GetTextInputClient())
    return;

  const string16 utf16_text = UTF8ToUTF16(text.text());
  if (utf16_text.empty())
    return;

  // Append the text to the buffer, because commit signal might be fired
  // multiple times when processing a key event.
  result_text_.append(utf16_text);

  // If we are not handling key event, do not bother sending text result if the
  // focused text input client does not support text input.
  if (pending_key_events_.empty() && !IsTextInputTypeNone()) {
    SendFakeProcessKeyEvent(true);
    GetTextInputClient()->InsertText(utf16_text);
    SendFakeProcessKeyEvent(false);
    result_text_.clear();
  }
}

void InputMethodIBus::OnForwardKeyEvent(uint32 keyval,
                                        uint32 keycode,
                                        uint32 state) {
  KeyboardCode ui_key_code = KeyboardCodeFromXKeysym(keyval);
  if (!ui_key_code)
    return;

  const EventType event_type =
      (state & kIBusReleaseMask) ? ET_KEY_RELEASED : ET_KEY_PRESSED;
  const int event_flags = EventFlagsFromXFlags(state);

  // It is not clear when the input method will forward us a fake key event.
  // If there is a pending key event, then we may already received some input
  // method results, so we dispatch this fake key event directly rather than
  // calling ProcessKeyEventPostIME(), which will clear pending input method
  // results.
  if (event_type == ET_KEY_PRESSED) {
    ProcessUnfilteredFabricatedKeyPressEvent(
        event_type, ui_key_code, event_flags, keyval);
  } else {
    DispatchFabricatedKeyEventPostIME(event_type, ui_key_code, event_flags);
  }
}

void InputMethodIBus::OnShowPreeditText() {
  if (suppress_next_result_ || IsTextInputTypeNone())
    return;

  composing_text_ = true;
}

void InputMethodIBus::OnUpdatePreeditText(const chromeos::ibus::IBusText& text,
                                          uint32 cursor_pos,
                                          bool visible) {
  if (suppress_next_result_ || IsTextInputTypeNone())
    return;

  // |visible| argument is very confusing. For example, what's the correct
  // behavior when:
  // 1. OnUpdatePreeditText() is called with a text and visible == false, then
  // 2. OnShowPreeditText() is called afterwards.
  //
  // If it's only for clearing the current preedit text, then why not just use
  // OnHidePreeditText()?
  if (!visible) {
    OnHidePreeditText();
    return;
  }

  ExtractCompositionText(text, cursor_pos, &composition_);

  composition_changed_ = true;

  // In case OnShowPreeditText() is not called.
  if (composition_.text.length())
    composing_text_ = true;

  // If we receive a composition text without pending key event, then we need to
  // send it to the focused text input client directly.
  if (pending_key_events_.empty()) {
    SendFakeProcessKeyEvent(true);
    GetTextInputClient()->SetCompositionText(composition_);
    SendFakeProcessKeyEvent(false);
    composition_changed_ = false;
    composition_.Clear();
  }
}

void InputMethodIBus::OnHidePreeditText() {
  if (composition_.text.empty() || IsTextInputTypeNone())
    return;

  // Intentionally leaves |composing_text_| unchanged.
  composition_changed_ = true;
  composition_.Clear();

  if (pending_key_events_.empty()) {
    TextInputClient* client = GetTextInputClient();
    if (client && client->HasCompositionText())
      client->ClearCompositionText();
    composition_changed_ = false;
  }
}

void InputMethodIBus::ResetInputContext() {
  context_focused_ = false;

  ConfirmCompositionText();

  // We are dead, so we need to ask the client to stop relying on us.
  OnInputMethodChanged();
  ibus_client_->DestroyProxy();
}

void InputMethodIBus::OnConnected() {
  DCHECK(ibus_client_->IsConnected());
  // If already input context is initialized, do nothing.
  if (ibus_client_->IsContextReady())
    return;

  DestroyContext();
  CreateContext();
}

void InputMethodIBus::OnDisconnected() {
  DestroyContext();
}

void InputMethodIBus::ExtractCompositionText(
    const chromeos::ibus::IBusText& text,
    uint32 cursor_position,
    CompositionText* out_composition) const {
  out_composition->Clear();
  out_composition->text = UTF8ToUTF16(text.text());

  if (out_composition->text.empty())
    return;

  // ibus uses character index for cursor position and attribute range, but we
  // use char16 offset for them. So we need to do conversion here.
  std::vector<size_t> char16_offsets;
  size_t length = out_composition->text.length();
  base::i18n::UTF16CharIterator char_iterator(&out_composition->text);
  do {
    char16_offsets.push_back(char_iterator.array_pos());
  } while (char_iterator.Advance());

  // The text length in Unicode characters.
  uint32 char_length = static_cast<uint32>(char16_offsets.size());
  // Make sure we can convert the value of |char_length| as well.
  char16_offsets.push_back(length);

  size_t cursor_offset =
      char16_offsets[std::min(char_length, cursor_position)];

  out_composition->selection = Range(cursor_offset);

  const std::vector<chromeos::ibus::IBusText::UnderlineAttribute>&
      underline_attributes = text.underline_attributes();
  const std::vector<chromeos::ibus::IBusText::SelectionAttribute>&
      selection_attributes = text.selection_attributes();

  if (!underline_attributes.empty()) {
    for (size_t i = 0; i < underline_attributes.size(); ++i) {
      const uint32 start = underline_attributes[i].start_index;
      const uint32 end = underline_attributes[i].end_index;
      if (start >= end)
        continue;
      CompositionUnderline underline(
          char16_offsets[start], char16_offsets[end],
          SK_ColorBLACK, false /* thick */);
      if (underline_attributes[i].type ==
          chromeos::ibus::IBusText::IBUS_TEXT_UNDERLINE_DOUBLE)
        underline.thick = true;
      else if (underline_attributes[i].type ==
               chromeos::ibus::IBusText::IBUS_TEXT_UNDERLINE_ERROR)
        underline.color = SK_ColorRED;
      out_composition->underlines.push_back(underline);
    }
  }

  if (!selection_attributes.empty()) {
    LOG_IF(ERROR, selection_attributes.size() != 1)
        << "Chrome does not support multiple selection";
    for (uint32 i = 0; i < selection_attributes.size(); ++i) {
      const uint32 start = selection_attributes[i].start_index;
      const uint32 end = selection_attributes[i].end_index;
      if (start >= end)
        continue;
      CompositionUnderline underline(
          char16_offsets[start], char16_offsets[end],
          SK_ColorBLACK, true /* thick */);
      out_composition->underlines.push_back(underline);
      // If the cursor is at start or end of this underline, then we treat
      // it as the selection range as well, but make sure to set the cursor
      // position to the selection end.
      if (underline.start_offset == cursor_offset) {
        out_composition->selection.set_start(underline.end_offset);
        out_composition->selection.set_end(cursor_offset);
      } else if (underline.end_offset == cursor_offset) {
        out_composition->selection.set_start(underline.start_offset);
        out_composition->selection.set_end(cursor_offset);
      }
    }
  }

  // Use a black thin underline by default.
  if (out_composition->underlines.empty()) {
    out_composition->underlines.push_back(CompositionUnderline(
        0, length, SK_ColorBLACK, false /* thick */));
  }
}

}  // namespace ui

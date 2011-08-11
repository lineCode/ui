// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/render_text.h"

#include <algorithm>

#include "base/i18n/break_iterator.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/canvas_skia.h"
#include "unicode/uchar.h"

namespace {

#ifndef NDEBUG
// Check StyleRanges invariant conditions: sorted and non-overlapping ranges.
void CheckStyleRanges(const gfx::StyleRanges& style_ranges, size_t length) {
  if (length == 0) {
    DCHECK(style_ranges.empty()) << "Style ranges exist for empty text.";
    return;
  }
  for (gfx::StyleRanges::size_type i = 0; i < style_ranges.size() - 1; i++) {
    const ui::Range& former = style_ranges[i].range;
    const ui::Range& latter = style_ranges[i + 1].range;
    DCHECK(!former.is_empty()) << "Empty range at " << i << ":" << former;
    DCHECK(former.IsValid()) << "Invalid range at " << i << ":" << former;
    DCHECK(!former.is_reversed()) << "Reversed range at " << i << ":" << former;
    DCHECK(former.end() == latter.start()) << "Ranges gap/overlap/unsorted." <<
        "former:" << former << ", latter:" << latter;
  }
  const gfx::StyleRange& end_style = *style_ranges.rbegin();
  DCHECK(!end_style.range.is_empty()) << "Empty range at end.";
  DCHECK(end_style.range.IsValid()) << "Invalid range at end.";
  DCHECK(!end_style.range.is_reversed()) << "Reversed range at end.";
  DCHECK(end_style.range.end() == length) << "Style and text length mismatch.";
}
#endif

void ApplyStyleRangeImpl(gfx::StyleRanges* style_ranges,
                         gfx::StyleRange style_range) {
  const ui::Range& new_range = style_range.range;
  // Follow StyleRanges invariant conditions: sorted and non-overlapping ranges.
  gfx::StyleRanges::iterator i;
  for (i = style_ranges->begin(); i != style_ranges->end();) {
    if (i->range.end() < new_range.start()) {
      i++;
    } else if (i->range.start() == new_range.end()) {
      break;
    } else if (new_range.Contains(i->range)) {
      i = style_ranges->erase(i);
      if (i == style_ranges->end())
        break;
    } else if (i->range.start() < new_range.start() &&
               i->range.end() > new_range.end()) {
      // Split the current style into two styles.
      gfx::StyleRange split_style = gfx::StyleRange(*i);
      split_style.range.set_end(new_range.start());
      i = style_ranges->insert(i, split_style) + 1;
      i->range.set_start(new_range.end());
      break;
    } else if (i->range.start() < new_range.start()) {
      i->range.set_end(new_range.start());
      i++;
    } else if (i->range.end() > new_range.end()) {
      i->range.set_start(new_range.end());
      break;
    } else
      NOTREACHED();
  }
  // Add the new range in its sorted location.
  style_ranges->insert(i, style_range);
}

}  // namespace

namespace gfx {

StyleRange::StyleRange()
    : font(),
      foreground(SK_ColorBLACK),
      strike(false),
      underline(false),
      range() {
}

SelectionModel::SelectionModel() {
  Init(0, 0, 0, PREVIOUS_GRAPHEME_TRAILING);
}

SelectionModel::SelectionModel(size_t pos) {
  Init(pos, pos, pos, PREVIOUS_GRAPHEME_TRAILING);
}

SelectionModel::SelectionModel(size_t end,
                               size_t pos,
                               CaretPlacement placement) {
  Init(end, end, pos, placement);
}

SelectionModel::SelectionModel(size_t start,
                               size_t end,
                               size_t pos,
                               CaretPlacement placement) {
  Init(start, end, pos, placement);
}

SelectionModel::~SelectionModel() {
}

bool SelectionModel::Equals(const SelectionModel& sel) const {
  return selection_start_ == sel.selection_start() &&
         selection_end_ == sel.selection_end() &&
         caret_pos_ == sel.caret_pos() &&
         caret_placement_ == sel.caret_placement();
}

void SelectionModel::Init(size_t start,
                          size_t end,
                          size_t pos,
                          CaretPlacement placement) {
  selection_start_ = start;
  selection_end_ = end;
  caret_pos_ = pos;
  caret_placement_ = placement;
}

RenderText::~RenderText() {
}

void RenderText::SetText(const string16& text) {
  size_t old_text_length = text_.length();
  text_ = text;

  // Update the style ranges as needed.
  if (text_.empty()) {
    style_ranges_.clear();
  } else if (style_ranges_.empty()) {
    ApplyDefaultStyle();
  } else if (text_.length() > old_text_length) {
    style_ranges_.back().range.set_end(text_.length());
  } else if (text_.length() < old_text_length) {
    StyleRanges::iterator i;
    for (i = style_ranges_.begin(); i != style_ranges_.end(); i++) {
      if (i->range.start() >= text_.length()) {
        i = style_ranges_.erase(i);
        if (i == style_ranges_.end())
          break;
      } else if (i->range.end() > text_.length()) {
        i->range.set_end(text_.length());
      }
    }
    style_ranges_.back().range.set_end(text_.length());
  }
#ifndef NDEBUG
  CheckStyleRanges(style_ranges_, text_.length());
#endif
  cached_bounds_and_offset_valid_ = false;
}

void RenderText::SetSelectionModel(const SelectionModel& sel) {
  size_t start = sel.selection_start();
  size_t end = sel.selection_end();
  selection_model_.set_selection_start(std::min(start, text().length()));
  selection_model_.set_selection_end(std::min(end, text().length()));
  selection_model_.set_caret_pos(std::min(sel.caret_pos(), text().length()));
  selection_model_.set_caret_placement(sel.caret_placement());

  cached_bounds_and_offset_valid_ = false;
}

void RenderText::SetDisplayRect(const Rect& r) {
  display_rect_ = r;
  cached_bounds_and_offset_valid_ = false;
}

size_t RenderText::GetCursorPosition() const {
  return selection_model_.selection_end();
}

void RenderText::SetCursorPosition(const size_t position) {
  SelectionModel sel(selection_model());
  sel.set_selection_start(position);
  sel.set_selection_end(position);
  sel.set_caret_pos(GetIndexOfPreviousGrapheme(position));
  sel.set_caret_placement(SelectionModel::TRAILING);
  SetSelectionModel(sel);
}

void RenderText::MoveCursorLeft(BreakType break_type, bool select) {
  SelectionModel position(selection_model());
  position.set_selection_start(GetCursorPosition());
  // Cancelling a selection moves to the edge of the selection.
  if (break_type != LINE_BREAK && !EmptySelection() && !select) {
    // Use the selection start if it is left of the selection end.
    SelectionModel selection_start(GetSelectionStart(), GetSelectionStart(),
                                   SelectionModel::LEADING);
    if (GetCursorBounds(selection_start, false).x() <
        GetCursorBounds(position, false).x())
      position = selection_start;
    // For word breaks, use the nearest word boundary left of the selection.
    if (break_type == WORD_BREAK)
      position = GetLeftSelectionModel(position, break_type);
  } else {
    position = GetLeftSelectionModel(position, break_type);
  }
  if (select)
    position.set_selection_start(GetSelectionStart());
  MoveCursorTo(position);
}

void RenderText::MoveCursorRight(BreakType break_type, bool select) {
  SelectionModel position(selection_model());
  position.set_selection_start(GetCursorPosition());
  // Cancelling a selection moves to the edge of the selection.
  if (break_type != LINE_BREAK && !EmptySelection() && !select) {
    // Use the selection start if it is right of the selection end.
    SelectionModel selection_start(GetSelectionStart(), GetSelectionStart(),
                                   SelectionModel::LEADING);
    if (GetCursorBounds(selection_start, false).x() >
        GetCursorBounds(position, false).x())
      position = selection_start;
    // For word breaks, use the nearest word boundary right of the selection.
    if (break_type == WORD_BREAK)
      position = GetRightSelectionModel(position, break_type);
  } else {
    position = GetRightSelectionModel(position, break_type);
  }
  if (select)
    position.set_selection_start(GetSelectionStart());
  MoveCursorTo(position);
}

bool RenderText::MoveCursorTo(const SelectionModel& selection) {
  bool changed = !selection.Equals(selection_model_);
  SetSelectionModel(selection);
  return changed;
}

bool RenderText::MoveCursorTo(const Point& point, bool select) {
  SelectionModel selection = FindCursorPosition(point);
  if (select)
    selection.set_selection_start(GetSelectionStart());
  return MoveCursorTo(selection);
}

bool RenderText::IsPointInSelection(const Point& point) {
  // TODO(xji): should this check whether the point is inside the visual
  // selection bounds? In case of "abcFED", if "ED" is selected, |point| points
  // to the right half of 'c', is the point in selection?
  size_t pos = FindCursorPosition(point).selection_end();
  return (pos >= MinOfSelection() && pos < MaxOfSelection());
}

void RenderText::ClearSelection() {
  SelectionModel sel(selection_model());
  sel.set_selection_start(GetCursorPosition());
  SetSelectionModel(sel);
}

void RenderText::SelectAll() {
  SelectionModel sel(0, text().length(),
                     text().length(), SelectionModel::LEADING);
  SetSelectionModel(sel);
}

void RenderText::SelectWord() {
  size_t selection_start = GetSelectionStart();
  size_t cursor_position = GetCursorPosition();
  // First we setup selection_start_ and selection_end_. There are so many cases
  // because we try to emulate what select-word looks like in a gtk textfield.
  // See associated testcase for different cases.
  if (cursor_position > 0 && cursor_position < text().length()) {
    if (u_isalnum(text()[cursor_position])) {
      selection_start = cursor_position;
      cursor_position++;
    } else
      selection_start = cursor_position - 1;
  } else if (cursor_position == 0) {
    selection_start = cursor_position;
    if (text().length() > 0)
      cursor_position++;
  } else {
    selection_start = cursor_position - 1;
  }

  // Now we move selection_start_ to beginning of selection. Selection boundary
  // is defined as the position where we have alpha-num character on one side
  // and non-alpha-num char on the other side.
  for (; selection_start > 0; selection_start--) {
    if (IsPositionAtWordSelectionBoundary(selection_start))
      break;
  }

  // Now we move selection_end_ to end of selection. Selection boundary
  // is defined as the position where we have alpha-num character on one side
  // and non-alpha-num char on the other side.
  for (; cursor_position < text().length(); cursor_position++) {
    if (IsPositionAtWordSelectionBoundary(cursor_position))
      break;
  }

  SelectionModel sel(selection_model());
  sel.set_selection_start(selection_start);
  sel.set_selection_end(cursor_position);
  sel.set_caret_placement(SelectionModel::PREVIOUS_GRAPHEME_TRAILING);
  SetSelectionModel(sel);
}

const ui::Range& RenderText::GetCompositionRange() const {
  return composition_range_;
}

void RenderText::SetCompositionRange(const ui::Range& composition_range) {
  CHECK(!composition_range.IsValid() ||
        ui::Range(0, text_.length()).Contains(composition_range));
  composition_range_.set_end(composition_range.end());
  composition_range_.set_start(composition_range.start());
}

void RenderText::ApplyStyleRange(StyleRange style_range) {
  const ui::Range& new_range = style_range.range;
  if (!new_range.IsValid() || new_range.is_empty())
    return;
  CHECK(!new_range.is_reversed());
  CHECK(ui::Range(0, text_.length()).Contains(new_range));
  ApplyStyleRangeImpl(&style_ranges_, style_range);
#ifndef NDEBUG
  CheckStyleRanges(style_ranges_, text_.length());
#endif
  // TODO(xji): only invalidate if font or underline changes.
  cached_bounds_and_offset_valid_ = false;
}

void RenderText::ApplyDefaultStyle() {
  style_ranges_.clear();
  StyleRange style = StyleRange(default_style_);
  style.range.set_end(text_.length());
  style_ranges_.push_back(style);
  cached_bounds_and_offset_valid_ = false;
}

base::i18n::TextDirection RenderText::GetTextDirection() const {
  // TODO(msw): Bidi implementation, intended to replace the functionality added
  //  in crrev.com/91881 (discussed in codereview.chromium.org/7324011).
  return base::i18n::LEFT_TO_RIGHT;
}

int RenderText::GetStringWidth() {
  return default_style_.font.GetStringWidth(text());
}

void RenderText::Draw(Canvas* canvas) {
  // Clip the canvas to the text display area.
  canvas->ClipRectInt(display_rect_.x(), display_rect_.y(),
                      display_rect_.width(), display_rect_.height());

  // Draw the selection.
  std::vector<Rect> selection(GetSubstringBounds(GetSelectionStart(),
                                                 GetCursorPosition()));
  SkColor selection_color =
      focused() ? kFocusedSelectionColor : kUnfocusedSelectionColor;
  for (std::vector<Rect>::const_iterator i = selection.begin();
       i < selection.end(); ++i) {
    Rect r(*i);
    canvas->FillRectInt(selection_color, r.x(), r.y(), r.width(), r.height());
  }

  // Create a temporary copy of the style ranges for composition and selection.
  StyleRanges style_ranges(style_ranges_);
  ApplyCompositionAndSelectionStyles(&style_ranges);

  // Draw the text.
  Rect bounds(display_rect_);
  bounds.Offset(GetUpdatedDisplayOffset());
  for (StyleRanges::const_iterator i = style_ranges.begin();
       i < style_ranges.end(); ++i) {
    const Font& font = !i->underline ? i->font :
        i->font.DeriveFont(0, i->font.GetStyle() | Font::UNDERLINED);
    string16 text = text_.substr(i->range.start(), i->range.length());
    bounds.set_width(font.GetStringWidth(text));
    canvas->DrawStringInt(text, font, i->foreground, bounds);

    // Draw the strikethrough.
    if (i->strike) {
      SkPaint paint;
      paint.setAntiAlias(true);
      paint.setStyle(SkPaint::kFill_Style);
      paint.setColor(i->foreground);
      paint.setStrokeWidth(kStrikeWidth);
      canvas->AsCanvasSkia()->drawLine(SkIntToScalar(bounds.x()),
                                       SkIntToScalar(bounds.bottom()),
                                       SkIntToScalar(bounds.right()),
                                       SkIntToScalar(bounds.y()),
                                       paint);
    }

    bounds.set_x(bounds.x() + bounds.width());
  }

  // Paint cursor. Replace cursor is drawn as rectangle for now.
  Rect cursor(GetUpdatedCursorBounds());
  if (cursor_visible() && focused() && !cursor.IsEmpty())
    canvas->DrawRectInt(kCursorColor, cursor.x(), cursor.y(),
                        cursor.width(), cursor.height());
}

SelectionModel RenderText::FindCursorPosition(const Point& point) {
  const Font& font = default_style_.font;
  int left = 0;
  int left_pos = 0;
  int right = font.GetStringWidth(text());
  int right_pos = text().length();

  int x = point.x() - (display_rect_.x() + GetUpdatedDisplayOffset().x());
  if (x <= left) return SelectionModel(left_pos);
  if (x >= right) return SelectionModel(right_pos);
  // binary searching the cursor position.
  // TODO(oshima): use the center of character instead of edge.
  // Binary search may not work for language like arabic.
  while (std::abs(static_cast<long>(right_pos - left_pos)) > 1) {
    int pivot_pos = left_pos + (right_pos - left_pos) / 2;
    int pivot = font.GetStringWidth(text().substr(0, pivot_pos));
    if (pivot < x) {
      left = pivot;
      left_pos = pivot_pos;
    } else if (pivot == x) {
      return SelectionModel(pivot_pos);
    } else {
      right = pivot;
      right_pos = pivot_pos;
    }
  }
  return SelectionModel(left_pos);
}

std::vector<Rect> RenderText::GetSubstringBounds(size_t from, size_t to) {
  size_t start = std::min(from, to);
  size_t end = std::max(from, to);
  const Font& font = default_style_.font;
  int start_x = font.GetStringWidth(text().substr(0, start));
  int end_x = font.GetStringWidth(text().substr(0, end));
  Rect rect(start_x, 0, end_x - start_x, font.GetHeight());
  rect.Offset(display_rect_.origin());
  rect.Offset(GetUpdatedDisplayOffset());
  // Center the rect vertically in |display_rect_|.
  rect.Offset(Point(0, (display_rect_.height() - rect.height()) / 2));
  return std::vector<Rect>(1, rect);
}

Rect RenderText::GetCursorBounds(const SelectionModel& selection,
                                 bool insert_mode) {
  size_t from = selection.selection_end();
  size_t to = insert_mode ? from : std::min(text_.length(), from + 1);
  return GetSubstringBounds(from, to)[0];
}

const Rect& RenderText::GetUpdatedCursorBounds() {
  UpdateCachedBoundsAndOffset();
  return cursor_bounds_;
}

RenderText::RenderText()
    : text_(),
      selection_model_(),
      cursor_bounds_(),
      cursor_visible_(false),
      insert_mode_(true),
      composition_range_(),
      style_ranges_(),
      default_style_(),
      display_rect_(),
      display_offset_(),
      cached_bounds_and_offset_valid_(false) {
}

const Point& RenderText::GetUpdatedDisplayOffset() {
  UpdateCachedBoundsAndOffset();
  return display_offset_;
}

SelectionModel RenderText::GetLeftSelectionModel(const SelectionModel& current,
                                                 BreakType break_type) {
  if (break_type == LINE_BREAK)
    return SelectionModel(0, 0, SelectionModel::LEADING);
  size_t pos = std::max(static_cast<long>(current.selection_end() - 1),
                        static_cast<long>(0));
  if (break_type == CHARACTER_BREAK)
    return SelectionModel(pos, pos, SelectionModel::LEADING);

  // Notes: We always iterate words from the begining.
  // This is probably fast enough for our usage, but we may
  // want to modify WordIterator so that it can start from the
  // middle of string and advance backwards.
  base::i18n::BreakIterator iter(text(), base::i18n::BreakIterator::BREAK_WORD);
  bool success = iter.Init();
  DCHECK(success);
  if (!success)
    return current;
  while (iter.Advance()) {
    if (iter.IsWord()) {
      size_t begin = iter.pos() - iter.GetString().length();
      if (begin == current.selection_end()) {
        // The cursor is at the beginning of a word.
        // Move to previous word.
        break;
      } else if (iter.pos() >= current.selection_end()) {
        // The cursor is in the middle or at the end of a word.
        // Move to the top of current word.
        pos = begin;
        break;
      } else {
        pos = iter.pos() - iter.GetString().length();
      }
    }
  }

  return SelectionModel(pos, pos, SelectionModel::LEADING);
}

SelectionModel RenderText::GetRightSelectionModel(const SelectionModel& current,
                                                  BreakType break_type) {
  if (break_type == LINE_BREAK)
    return SelectionModel(text().length(),
        GetIndexOfPreviousGrapheme(text().length()), SelectionModel::TRAILING);
  size_t pos = std::min(current.selection_end() + 1, text().length());
  if (break_type == CHARACTER_BREAK)
    return SelectionModel(pos, pos, SelectionModel::LEADING);

  base::i18n::BreakIterator iter(text(), base::i18n::BreakIterator::BREAK_WORD);
  bool success = iter.Init();
  DCHECK(success);
  if (!success)
    return current;
  while (iter.Advance()) {
    pos = iter.pos();
    if (iter.IsWord() && pos > current.selection_end())
      break;
  }
  return SelectionModel(pos, pos, SelectionModel::LEADING);
}

size_t RenderText::GetIndexOfPreviousGrapheme(size_t position) const {
  // TODO(msw): Handle complex script.
  return std::max(static_cast<int>(position - 1), static_cast<int>(0));
}

void RenderText::ApplyCompositionAndSelectionStyles(
    StyleRanges* style_ranges) const {
  // TODO(msw): This pattern ought to be reconsidered; what about composition
  //            and selection overlaps, retain existing local style features?
  // Apply a composition style override to a copy of the style ranges.
  if (composition_range_.IsValid() && !composition_range_.is_empty()) {
    StyleRange composition_style(default_style_);
    composition_style.underline = true;
    composition_style.range.set_start(composition_range_.start());
    composition_style.range.set_end(composition_range_.end());
    ApplyStyleRangeImpl(style_ranges, composition_style);
  }
  // Apply a selection style override to a copy of the style ranges.
  if (!EmptySelection()) {
    StyleRange selection_style(default_style_);
    selection_style.foreground = kSelectedTextColor;
    selection_style.range.set_start(MinOfSelection());
    selection_style.range.set_end(MaxOfSelection());
    ApplyStyleRangeImpl(style_ranges, selection_style);
  }
}

bool RenderText::IsPositionAtWordSelectionBoundary(size_t pos) {
  return pos == 0 || (u_isalnum(text()[pos - 1]) && !u_isalnum(text()[pos])) ||
      (!u_isalnum(text()[pos - 1]) && u_isalnum(text()[pos]));
}

void RenderText::UpdateCachedBoundsAndOffset() {
  if (cached_bounds_and_offset_valid_)
    return;
  // First, set the valid flag true to calculate the current cursor bounds using
  // the stale |display_offset_|. Applying |delta_offset| at the end of this
  // function will set |cursor_bounds_| and |display_offset_| to correct values.
  cached_bounds_and_offset_valid_ = true;
  cursor_bounds_ = GetCursorBounds(selection_model_, insert_mode_);
  cursor_bounds_.set_width(std::max(cursor_bounds_.width(), 1));
  // Update |display_offset_| to ensure the current cursor is visible.
  int display_width = display_rect_.width();
  int string_width = GetStringWidth();
  int delta_offset = 0;
  if (string_width < display_width) {
    // Show all text whenever the text fits to the size.
    delta_offset = -display_offset_.x();
  } else if (cursor_bounds_.right() > display_rect_.right()) {
    // Pan to show the cursor when it overflows to the right,
    delta_offset = display_rect_.right() - cursor_bounds_.right();
  } else if (cursor_bounds_.x() < display_rect_.x()) {
    // Pan to show the cursor when it overflows to the left.
    delta_offset = display_rect_.x() - cursor_bounds_.x();
  }
  display_offset_.Offset(delta_offset, 0);
  cursor_bounds_.Offset(delta_offset, 0);
}

}  // namespace gfx

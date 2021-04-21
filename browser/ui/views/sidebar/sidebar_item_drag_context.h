/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_DRAG_CONTEXT_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_DRAG_CONTEXT_H_

#include "ui/gfx/geometry/point.h"

namespace ui {
class LocatedEvent;
}  // namespace ui

namespace views {
class View;
}  // namespace views

class SidebarItemDragContext final {
 public:
  SidebarItemDragContext();
  ~SidebarItemDragContext();
  SidebarItemDragContext(const SidebarItemDragContext&) = delete;
  SidebarItemDragContext& operator=(const SidebarItemDragContext&) = delete;

  void MaybeStartDrag(views::View* view, const ui::LocatedEvent& event);
  void ContinueDrag(views::View* view, const ui::LocatedEvent& event);
  void EndDrag();

  bool ShouldMoveItem() const;

  bool is_dragging() const { return is_dragging_; }

  void set_source_index(int index) { source_index_ = index; }
  void set_drag_indicator_index(int index) { drag_indicator_index_ = index; }
  int source_index() const { return source_index_; }
  int GetTargetIndex() const;

 private:
  bool is_dragging_ = false;
  int source_index_ = -1;
  int drag_indicator_index_ = -1;
  gfx::Point start_point_in_screen_;

  bool CanStartDrag(const gfx::Point& point_in_screen);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_DRAG_CONTEXT_H_

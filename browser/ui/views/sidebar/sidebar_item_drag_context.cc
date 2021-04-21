/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_drag_context.h"

#include "ui/events/event.h"
#include "ui/views/view.h"

SidebarItemDragContext::SidebarItemDragContext() = default;

SidebarItemDragContext::~SidebarItemDragContext() = default;

int SidebarItemDragContext::GetTargetIndex() const {
  return drag_indicator_index_ > source_index_ ? drag_indicator_index_ - 1
                                               : drag_indicator_index_;
}

bool SidebarItemDragContext::ShouldMoveItem() const {
  return drag_indicator_index_ != -1;
}

void SidebarItemDragContext::MaybeStartDrag(views::View* view,
                                            const ui::LocatedEvent& event) {
  is_dragging_ = false;
  source_index_ = -1;
  drag_indicator_index_ = -1;
  start_point_in_screen_ = event.location();
  views::View::ConvertPointToScreen(view, &start_point_in_screen_);
}

void SidebarItemDragContext::ContinueDrag(views::View* view,
                                          const ui::LocatedEvent& event) {
  gfx::Point screen_position = event.location();
  views::View::ConvertPointToScreen(view, &screen_position);

  if (!is_dragging_ && CanStartDrag(screen_position)) {
    is_dragging_ = true;
    DVLOG(2) << __func__ << ": Start dragging";
  }
}

void SidebarItemDragContext::EndDrag() {
}

bool SidebarItemDragContext::CanStartDrag(const gfx::Point& point_in_screen) {
  // Determine if the mouse has moved beyond a minimum elasticity distance in
  // any direction from the starting point.
  constexpr int kMinimumDragDistance = 10;
  int x_offset = abs(point_in_screen.x() - start_point_in_screen_.x());
  int y_offset = abs(point_in_screen.y() - start_point_in_screen_.y());
  return sqrt(pow(float{x_offset}, 2) + pow(float{y_offset}, 2)) >
         kMinimumDragDistance;
}

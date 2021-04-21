/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"

#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_controller.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"

SidebarItemView::SidebarItemView(Delegate* delegate,
                                 SidebarItemController* controller)
    : SidebarButtonView(delegate), controller_(controller) {
  DCHECK(controller_);
}

SidebarItemView::~SidebarItemView() = default;

void SidebarItemView::DrawHorizontalBorder(bool top) {
  DCHECK(!draw_horizontal_border_);

  draw_horizontal_border_ = true;
  draw_horizontal_border_top_ = top;
  SchedulePaint();
}

void SidebarItemView::ClearHorizontalBorder() {
  if (!draw_horizontal_border_)
    return;

  draw_horizontal_border_ = false;
  SchedulePaint();
}

void SidebarItemView::OnPaintBorder(gfx::Canvas* canvas) {
  ImageButton::OnPaintBorder(canvas);

  // Draw item highlight
  if (draw_highlight_) {
    auto& bundle = ui::ResourceBundle::GetSharedInstance();
    canvas->DrawImageInt(*bundle.GetImageSkiaNamed(IDR_SIDEBAR_ITEM_HIGHLIGHT),
                         0, 0);
  }

  if (draw_horizontal_border_) {
    constexpr float kHorizontalBorderWidth = 2;
    gfx::Rect border_rect(GetLocalBounds());

    if (!draw_horizontal_border_top_)
      border_rect.set_y(border_rect.bottom() - kHorizontalBorderWidth);

    border_rect.set_height(kHorizontalBorderWidth);

    canvas->FillRect(border_rect, SkColorSetRGB(0x21, 0x25, 0x29));
  }
}

void SidebarItemView::OnPaintBackground(gfx::Canvas* canvas) {
  SidebarButtonView::OnPaintBackground(canvas);

  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    if (paint_background_on_hovered_ && GetState() == STATE_HOVERED) {
      canvas->FillRect(
          GetLocalBounds(),
          theme_provider->GetColor(
              BraveThemeProperties::COLOR_SIDEBAR_ITEM_BACKGROUND));
    }
  }
}

bool SidebarItemView::OnMousePressed(const ui::MouseEvent& event) {
  controller_->MaybeStartDrag(this, event);
  return true;
}

bool SidebarItemView::OnMouseDragged(const ui::MouseEvent& event) {
  controller_->ContinueDrag(this, event);
  return true;
}

void SidebarItemView::OnMouseReleased(const ui::MouseEvent& event) {
  SidebarButtonView::OnMouseReleased(event);

  controller_->EndDrag(true);
}

void SidebarItemView::OnMouseCaptureLost() {
  controller_->EndDrag(false);
}

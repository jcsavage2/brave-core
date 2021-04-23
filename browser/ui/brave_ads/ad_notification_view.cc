/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/ad_notification_view.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/ui/brave_ads/ad_notification_background_painter.h"
#include "brave/browser/ui/brave_ads/ad_notification_popup.h"
#include "brave/browser/ui/brave_ads/bounds_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "build/build_config.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/screen.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/shadow_util.h"
#include "ui/gfx/shadow_value.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/metadata/metadata_impl_macros.h"
#include "ui/views/view.h"

#if defined(OS_WIN)
#include "ui/base/win/shell.h"
#endif

namespace brave_ads {

namespace {

constexpr SkColor kBackgroundColor = SkColorSetRGB(0xed, 0xf0, 0xf2);
constexpr SkColor kBorderColor = SkColorSetRGB(0xd5, 0xdb, 0xe2);

#if defined(OS_WIN)
const int kCornerRadius = 0;
#elif defined(OS_MAC)
const int kCornerRadius = 7;
#elif defined(OS_LINUX)
const int kCornerRadius = 7;
#endif

const int kBorderThickness = 1;

const int kWindowsShadowElevation = 2;
const int kWindowsShadowRadius = 0;

bool ShouldShowAeroShadowBorder() {
#if defined(OS_WIN)
  return ui::win::IsAeroGlassEnabled();
#else
  return false;
#endif
}

}  // namespace

AdNotificationView::AdNotificationView(const AdNotification& ad_notification)
    : ad_notification_(ad_notification) {
  CreateView();
}

AdNotificationView::~AdNotificationView() = default;

void AdNotificationView::UpdateContents(const AdNotification& ad_notification) {
  ad_notification_ = ad_notification;

  UpdateView();

  MaybeNotifyAccessibilityEvent(ad_notification);
}

void AdNotificationView::OnCloseButtonPressed() {
  if (is_closing_) {
    return;
  }

  is_closing_ = true;

  const std::string id = ad_notification_.id();
  AdNotificationPopup::Close(id, /* by_user */ true);
}

void AdNotificationView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kGenericContainer;
  node_data->AddStringAttribute(
      ax::mojom::StringAttribute::kRoleDescription,
      l10n_util::GetStringUTF8(IDS_BRAVE_ADS_AD_NOTIFICATION_ACCESSIBLE_NAME));

  if (accessible_name_.empty()) {
    node_data->SetNameFrom(ax::mojom::NameFrom::kAttributeExplicitlyEmpty);
  }

  node_data->SetName(accessible_name_);
}

bool AdNotificationView::OnMousePressed(const ui::MouseEvent& event) {
  initial_mouse_pressed_location_ = event.location();

  return true;
}

bool AdNotificationView::OnMouseDragged(const ui::MouseEvent& event) {
  const gfx::Vector2d movement =
      event.location() - initial_mouse_pressed_location_;

  if (!is_dragging_ && ExceededDragThreshold(movement)) {
    is_dragging_ = true;
  }

  if (!is_dragging_) {
    return false;
  }

  gfx::Rect bounds = GetBoundsInScreen() + movement;
  const gfx::NativeView native_view = GetWidget()->GetNativeView();
  AdjustBoundsToFitWorkAreaForNativeView(&bounds, native_view);

  GetWidget()->SetBounds(bounds);

  return true;
}

void AdNotificationView::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    return;
  }

  if (!event.IsOnlyLeftMouseButton()) {
    return;
  }

  const std::string id = ad_notification_.id();
  AdNotificationPopup::OnClick(id);

  View::OnMouseReleased(event);
}

void AdNotificationView::OnPaint(gfx::Canvas* canvas) {
  DCHECK(canvas);

  if (ShouldShowAeroShadowBorder()) {
    // If the border is a shadow, paint the border first
    OnPaintBorder(canvas);

    // Clip at the border so we do not paint over it
    canvas->ClipRect(GetContentsBounds());

    OnPaintBackground(canvas);
  } else {
    View::OnPaint(canvas);
  }
}

void AdNotificationView::OnBlur() {
  View::OnBlur();

  UpdateView();
}

void AdNotificationView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateView();
}

///////////////////////////////////////////////////////////////////////////////

void AdNotificationView::CreateView() {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Paint to a dedicated layer to make the layer non-opaque
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  UpdateContents(ad_notification_);

  // If Aero is enabled, set shadow border
  if (ShouldShowAeroShadowBorder()) {
    const gfx::ShadowDetails& shadow_details =
        gfx::ShadowDetails::Get(kWindowsShadowElevation, kWindowsShadowRadius);

    const gfx::Insets insets =
        gfx::ShadowValue::GetBlurRegion(shadow_details.values);

    SetBorder(views::CreateBorderPainter(
        views::Painter::CreateImagePainter(shadow_details.ninebox_image,
                                           insets),
        -gfx::ShadowValue::GetMargin(shadow_details.values)));
  }
}

void AdNotificationView::UpdateView() {
  SetBackground(views::CreateBackgroundFromPainter(
      std::make_unique<AdNotificationBackgroundPainter>(
          kCornerRadius, kCornerRadius, kBackgroundColor)));

  SetBorder(views::CreateRoundedRectBorder(kBorderThickness, kCornerRadius,
                                           kBorderColor));

  SchedulePaint();
}

void AdNotificationView::MaybeNotifyAccessibilityEvent(
    const AdNotification& ad_notification) {
  const base::string16 accessible_name = ad_notification.accessible_name();
  if (accessible_name != accessible_name_) {
    accessible_name_ = accessible_name;

    NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
  }
}

BEGIN_METADATA(AdNotificationView, views::View)
END_METADATA

}  // namespace brave_ads

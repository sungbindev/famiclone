/*
 * ------------------------------------------------------------------------
 * 名称：tokens.generated.hpp
 * 说明：设计令牌的最终取值，由 tools/gen-tokens.mjs 从 rujian-ui-kit 的令牌与主题生成：长度、数字、时间等是常量，颜色与阴影按主题放在 ThemeTokens 里。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：不要手改；令牌或组件引用变化后重新运行生成器，npm run win32:check 会检查这份文件与生成结果一致。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <string_view>

#include "rujian/ui/token-types.hpp"

namespace rujian::ui::tokens {

// 长度（逻辑像素）、数字、时间（毫秒）、百分比与 em 倍数。
inline constexpr double action_gap = 6.0;
inline constexpr double btn_add_min_width = 0.0;
inline constexpr double btn_add_padding_x = 8.0;
inline constexpr double btn_compact_min_width = 60.0;
inline constexpr double btn_delete_min_width = 0.0;
inline constexpr double btn_delete_padding_x = 8.0;
inline constexpr double btn_edit_min_width = 0.0;
inline constexpr double btn_edit_padding_x = 8.0;
inline constexpr double btn_font_size = 12.0;
inline constexpr double btn_font_weight = 400.0;
inline constexpr double btn_height = 22.0;
inline constexpr double btn_icon_size = 14.0;
inline constexpr double btn_padding_x = 8.0;
inline constexpr double btn_radius = 3.0;
inline constexpr double callout_font_size = 11.0;
inline constexpr double callout_line_height = 1.45;
inline constexpr double callout_padding_x = 10.0;
inline constexpr double callout_padding_y = 6.0;
inline constexpr double callout_title_font_weight = 600.0;
inline constexpr double card_radius = 6.0;
inline constexpr double chip_font_size = 11.0;
inline constexpr double chip_padding_x = 8.0;
inline constexpr double chip_padding_y = 1.0;
inline constexpr double choice_description_size = 10.0;
inline constexpr double choice_gap = 8.0;
inline constexpr double choice_group_gap = 6.0;
inline constexpr double choice_mark_size = 14.0;
inline constexpr double choice_min_height = 24.0;
inline constexpr double content_padding = 10.0;
inline constexpr double danger_notice_name_font_weight = 600.0;
inline constexpr double dialog_auto_max_height = 640.0;
inline constexpr double dialog_button_min_width = 72.0;
inline constexpr double dialog_control_size = 24.0;
inline constexpr double dialog_layer_z_index = 1.0;
inline constexpr double dialog_resizer_corner = 12.0;
inline constexpr double dialog_resizer_size = 6.0;
inline constexpr double dialog_title_font_size = 12.0;
inline constexpr double dialog_title_font_weight = 600.0;
inline constexpr double dialog_titlebar_height = 36.0;
inline constexpr double empty_state_action_icon_size = 16.0;
inline constexpr double empty_state_entry_font_size = 11.0;
inline constexpr double empty_state_gap = 10.0;
inline constexpr double empty_state_step_font_size = 13.0;
inline constexpr double empty_state_step_height = 28.0;
inline constexpr double empty_state_steps_width = 320.0;
inline constexpr double empty_state_subtitle_font_size = 12.0;
inline constexpr double empty_state_title_font_size = 16.0;
inline constexpr double empty_state_title_font_weight = 600.0;
inline constexpr double facts_label_font_size = 11.0;
inline constexpr double facts_label_width = 88.0;
inline constexpr double facts_row_gap = 4.0;
inline constexpr double facts_value_font_size = 12.0;
inline constexpr double field_description_emphasis_weight = 600.0;
inline constexpr double field_description_gap = 6.0;
inline constexpr double field_description_line_height = 1.45;
inline constexpr double field_description_size = 11.0;
inline constexpr double field_error_size = 11.0;
inline constexpr double field_font_size = 12.0;
inline constexpr double field_label_gap = 2.0;
inline constexpr double field_label_weight = 600.0;
inline constexpr double font_size_body = 12.0;
inline constexpr double font_size_caption = 11.0;
inline constexpr double font_size_chrome = 13.0;
inline constexpr double font_size_heading = 12.0;
inline constexpr double font_size_subheading = 11.0;
inline constexpr double font_size_title = 13.0;
inline constexpr double group_title_font_size = 11.0;
inline constexpr double group_title_font_weight = 600.0;
inline constexpr double group_title_padding_bottom = 4.0;
inline constexpr double heading_font_weight = 600.0;
inline constexpr double hover_duration_ms = 150.0;
inline constexpr double icon_size = 14.0;
inline constexpr double icon_slot_size = 16.0;
inline constexpr double icon_stroke_width = 1.0;
inline constexpr double icon_text_gap = 4.0;
inline constexpr double input_field_height = 22.0;
inline constexpr double input_font_size = 11.0;
inline constexpr double input_height = 24.0;
inline constexpr double input_padding_x = 8.0;
inline constexpr double input_prefix_gap = 4.0;
inline constexpr double input_prefix_max_width_percent = 60.0;
inline constexpr double input_radius = 4.0;
inline constexpr double keybinding_key_line_height = 10.0;
inline constexpr double keybinding_key_min_width = 12.0;
inline constexpr double line_height_body = 1.45;
inline constexpr double list_compact_gap = 2.0;
inline constexpr double list_compact_padding_x = 6.0;
inline constexpr double list_compact_padding_y = 2.0;
inline constexpr double list_divided_padding_y = 6.0;
inline constexpr double list_gap = 4.0;
inline constexpr double list_item_padding_x = 8.0;
inline constexpr double list_item_padding_y = 6.0;
inline constexpr double list_item_radius = 4.0;
inline constexpr double list_quiet_gap = 2.0;
inline constexpr double list_quiet_padding_x = 6.0;
inline constexpr double list_quiet_padding_y = 4.0;
inline constexpr double media_body_min_width = 240.0;
inline constexpr double media_gap = 10.0;
inline constexpr double menu_font_size = 13.0;
inline constexpr double menu_item_height = 24.0;
inline constexpr double menu_item_margin_x = 4.0;
inline constexpr double menu_item_padding_right = 26.0;
inline constexpr double menu_item_radius = 6.0;
inline constexpr double menu_mark_column_width = 26.0;
inline constexpr double menu_mark_size = 14.0;
inline constexpr double menu_popup_radius = 8.0;
inline constexpr double menu_separator_margin_y = 5.0;
inline constexpr double menu_shortcut_gap = 52.0;
inline constexpr double menubar_button_height = 24.0;
inline constexpr double menubar_button_padding = 8.0;
inline constexpr double menubar_button_radius = 6.0;
inline constexpr double motion_duration_ms = 180.0;
inline constexpr double notice_offset_bottom = 10.0;
inline constexpr double notice_toast_lines = 3.0;
inline constexpr double notice_width = 340.0;
inline constexpr double notice_z_index = 2.0;
inline constexpr double notification_badge_font_size = 11.0;
inline constexpr double notification_center_empty_font_size = 11.0;
inline constexpr double notification_center_item_lines = 4.0;
inline constexpr double notification_center_max_height = 480.0;
inline constexpr double notification_center_time_font_size = 11.0;
inline constexpr double notification_center_title_font_size = 11.0;
inline constexpr double notification_center_title_font_weight = 600.0;
inline constexpr double notification_center_title_line_height = 1.3;
inline constexpr double panel_compact_head_padding_y = 4.0;
inline constexpr double panel_compact_header_height = 28.0;
inline constexpr double panel_head_padding_y = 6.0;
inline constexpr double panel_header_height = 44.0;
inline constexpr double panel_padding_x = 10.0;
inline constexpr double panel_padding_y = 10.0;
inline constexpr double panel_subtitle_font_size = 11.0;
inline constexpr double panel_subtitle_line_height = 1.45;
inline constexpr double popup_z_index = 3.0;
inline constexpr double radius_lg = 6.0;
inline constexpr double radius_md = 4.0;
inline constexpr double radius_sm = 3.0;
inline constexpr double radius_xl = 8.0;
inline constexpr double row_list_column_gap = 10.0;
inline constexpr double row_list_description_font_size = 11.0;
inline constexpr double row_list_font_size = 12.0;
inline constexpr double row_list_gap = 0.0;
inline constexpr double row_list_leading_width = 96.0;
inline constexpr double row_list_min_height = 26.0;
inline constexpr double row_list_padding_x = 10.0;
inline constexpr double row_list_padding_y = 2.0;
inline constexpr double row_list_radius = 4.0;
inline constexpr double scrollbar_slider_radius = 999.0;
inline constexpr double section_title_font_size = 13.0;
inline constexpr double section_title_font_weight = 600.0;
inline constexpr double section_title_padding_bottom = 6.0;
inline constexpr double segmented_font_size = 12.0;
inline constexpr double segmented_height = 32.0;
inline constexpr double segmented_indicator_height = 2.0;
inline constexpr double segmented_padding_x = 10.0;
inline constexpr double segmented_selected_font_weight = 600.0;
inline constexpr double select_font_size = 11.0;
inline constexpr double select_height = 24.0;
inline constexpr double select_min_width = 88.0;
inline constexpr double select_option_description_size = 10.0;
inline constexpr double select_option_label_weight = 600.0;
inline constexpr double select_option_mark_gap = 6.0;
inline constexpr double select_option_mark_size = 14.0;
inline constexpr double select_option_padding_x = 8.0;
inline constexpr double select_option_padding_y = 4.0;
inline constexpr double select_padding_left = 8.0;
inline constexpr double select_padding_right = 24.0;
inline constexpr double select_popup_padding = 4.0;
inline constexpr double select_preview_image_radius = 4.0;
inline constexpr double select_preview_max_height = 160.0;
inline constexpr double select_preview_padding = 4.0;
inline constexpr double select_preview_width = 160.0;
inline constexpr double select_quiet_font_size = 12.0;
inline constexpr double select_quiet_height = 22.0;
inline constexpr double select_quiet_padding_x = 8.0;
inline constexpr double select_quiet_radius = 3.0;
inline constexpr double setting_row_control_width = 280.0;
inline constexpr double setting_row_description_font_size = 11.0;
inline constexpr double setting_row_description_line_height = 1.45;
inline constexpr double setting_row_label_font_size = 11.0;
inline constexpr double setting_row_label_font_weight = 600.0;
inline constexpr double setting_row_padding_y = 8.0;
inline constexpr double setting_row_text_width = 420.0;
inline constexpr double setting_row_value_label_width = 120.0;
inline constexpr double slider_height = 22.0;
inline constexpr double slider_thumb_size = 12.0;
inline constexpr double slider_track_height = 4.0;
inline constexpr double slider_width = 96.0;
inline constexpr double space_hairline = 1.0;
inline constexpr double space_md = 8.0;
inline constexpr double space_page = 10.0;
inline constexpr double space_sm = 6.0;
inline constexpr double space_xs = 4.0;
inline constexpr double space_xxs = 2.0;
inline constexpr double split_view_main_gap = 10.0;
inline constexpr double split_view_side_head_gap = 4.0;
inline constexpr double split_view_side_head_height = 28.0;
inline constexpr double split_view_side_head_padding_x = 4.0;
inline constexpr double split_view_side_title_font_size = 12.0;
inline constexpr double split_view_side_title_font_weight = 600.0;
inline constexpr double splitter_hover_delay_ms = 200.0;
inline constexpr double splitter_size = 4.0;
inline constexpr double state_text_font_size = 11.0;
inline constexpr double state_text_line_height = 1.45;
inline constexpr double status_bar_dot_size = 8.0;
inline constexpr double status_bar_font_size = 12.0;
inline constexpr double status_bar_height = 26.0;
inline constexpr double status_bar_icon_size = 16.0;
inline constexpr double status_bar_item_height = 22.0;
inline constexpr double status_bar_item_padding_x = 4.0;
inline constexpr double status_bar_lamp_height = 4.0;
inline constexpr double status_bar_lamp_width = 8.0;
inline constexpr double status_bar_large_dot_size = 9.0;
inline constexpr double status_bar_large_font_size = 13.0;
inline constexpr double status_bar_large_height = 32.0;
inline constexpr double status_bar_large_icon_size = 18.0;
inline constexpr double status_bar_large_item_height = 26.0;
inline constexpr double status_bar_large_item_padding_x = 6.0;
inline constexpr double status_bar_large_lamp_height = 4.0;
inline constexpr double status_bar_large_lamp_width = 9.0;
inline constexpr double status_bar_padding_x = 8.0;
inline constexpr double subheading_font_weight = 600.0;
inline constexpr double switch_height = 20.0;
inline constexpr double switch_list_row_min_height = 38.0;
inline constexpr double switch_row_gap = 8.0;
inline constexpr double switch_row_min_height = 0.0;
inline constexpr double switch_row_padding = 0.0;
inline constexpr double switch_thumb_size = 14.0;
inline constexpr double switch_width = 36.0;
inline constexpr double tab_count_font_size = 11.0;
inline constexpr double tab_count_font_weight = 400.0;
inline constexpr double tab_font_size = 12.0;
inline constexpr double tab_height = 32.0;
inline constexpr double tab_indicator_height = 2.0;
inline constexpr double tab_padding_x = 10.0;
inline constexpr double tab_selected_font_weight = 600.0;
inline constexpr double table_cell_padding_x = 10.0;
inline constexpr double table_cell_padding_y = 4.0;
inline constexpr double table_compact_cell_padding_x = 10.0;
inline constexpr double table_compact_cell_padding_y = 4.0;
inline constexpr double table_compact_font_size = 11.0;
inline constexpr double table_compact_row_height = 32.0;
inline constexpr double table_font_size = 12.0;
inline constexpr double table_head_padding_y = 6.0;
inline constexpr double table_header_font_size = 11.0;
inline constexpr double table_header_font_weight = 600.0;
inline constexpr double table_radius = 8.0;
inline constexpr double table_row_height = 40.0;
inline constexpr double table_sticky_top = 0.0;
inline constexpr double table_title_font_size = 12.0;
inline constexpr double table_title_font_weight = 600.0;
inline constexpr double tabs_steps_height = 44.0;
inline constexpr double title_bar_brand_font_weight = 700.0;
inline constexpr double title_bar_height = 36.0;
inline constexpr double title_bar_logo_size = 16.0;
inline constexpr double title_bar_subtitle_font_weight = 500.0;
inline constexpr double title_bar_title_letter_spacing_em = 0.08;
inline constexpr double title_bar_tool_gap = 10.0;
inline constexpr double title_bar_tool_width = 46.0;
inline constexpr double title_font_weight = 600.0;
inline constexpr double toggle_chip_font_size = 12.0;
inline constexpr double toggle_chip_gap = 4.0;
inline constexpr double toggle_chip_height = 22.0;
inline constexpr double toggle_chip_icon_size = 14.0;
inline constexpr double toggle_chip_mark_size = 10.0;
inline constexpr double toggle_chip_padding_x = 8.0;
inline constexpr double toggle_chip_quiet_height = 22.0;
inline constexpr double toggle_chip_quiet_padding_x = 8.0;
inline constexpr double toggle_chip_quiet_radius = 3.0;
inline constexpr double toggle_chip_radius = 6.0;
inline constexpr double toolbar_description_font_size = 11.0;
inline constexpr double toolbar_gap = 8.0;
inline constexpr double toolbar_min_height = 28.0;
inline constexpr double toolbar_padding_bottom = 6.0;
inline constexpr double toolbar_title_font_weight = 600.0;
inline constexpr double tooltip_arrow_size = 8.0;
inline constexpr double tooltip_head_icon_size = 16.0;
inline constexpr double tooltip_item_title_font_weight = 600.0;
inline constexpr double tooltip_max_width = 320.0;
inline constexpr double tooltip_padding_x = 8.0;
inline constexpr double tooltip_padding_y = 4.0;
inline constexpr double tooltip_rich_max_width = 380.0;
inline constexpr double tooltip_rich_padding_x = 10.0;
inline constexpr double tooltip_rich_padding_y = 8.0;
inline constexpr double tooltip_title_font_size = 13.0;
inline constexpr double tooltip_title_font_weight = 600.0;
inline constexpr double tree_count_font_size = 11.0;
inline constexpr double tree_font_size = 12.0;
inline constexpr double tree_indent = 12.0;
inline constexpr double tree_padding_x = 4.0;
inline constexpr double tree_radius = 4.0;
inline constexpr double tree_row_height = 22.0;

// 数字列表（如简写的内边距，按书写顺序）。
inline constexpr std::array<double, 2> keybinding_key_padding{3.0, 5.0};
inline constexpr std::array<double, 2> menu_popup_padding{4.0, 0.0};
inline constexpr std::array<double, 2> switch_list_row_padding{0.0, 10.0};

// 字体列表（按书写顺序，含通用字体族关键字）。
inline constexpr std::array<std::string_view, 6> font_family{"Rujian CJK Punctuation", "Segoe UI", "system-ui", "-apple-system", "Microsoft YaHei", "sans-serif"};
inline constexpr std::array<std::string_view, 7> title_bar_title_font_family{"Rujian Quicksand", "Rujian CJK Punctuation", "Segoe UI", "system-ui", "-apple-system", "Microsoft YaHei", "sans-serif"};

/** 随主题变化的令牌：颜色与阴影。 */
struct ThemeTokens {
  Color accent;
  Color accent_active;
  Color accent_foreground;
  Color accent_hover;
  Color bg;
  Color button_accent_active_bg;
  Color button_accent_bg;
  Color button_accent_hover_bg;
  Color button_accent_text;
  Color button_danger_active_bg;
  Color button_danger_bg;
  Color button_danger_hover_bg;
  Color button_danger_text;
  Color button_secondary_active_bg;
  Color button_secondary_bg;
  Color button_secondary_hover_bg;
  Color button_secondary_text;
  Color callout_color;
  Color callout_text_color;
  Color card_bg;
  Color card_border;
  ShadowList card_shadow;
  Color chip_bg;
  Color chip_border;
  Color choice_description_color;
  Color choice_mark_active_bg;
  Color choice_mark_bg;
  Color choice_mark_border;
  Color choice_mark_border_active;
  Color choice_mark_border_hover;
  Color choice_mark_hover_bg;
  Color choice_selected_bg;
  Color chrome_bg;
  Color chrome_border;
  Color chrome_text;
  Color chrome_text_strong;
  Color content_bg;
  Color danger_notice_bg;
  Color danger_notice_border;
  Color danger_notice_name_bg;
  Color danger_notice_text;
  Color dialog_close_active_bg;
  Color dialog_close_hover_bg;
  Color dialog_close_hover_text;
  Color dialog_glass_bg;
  Color dialog_glass_titlebar_bg;
  Color dialog_glass_titlebar_inactive_bg;
  ShadowList dialog_shadow;
  Color dialog_titlebar_bg;
  Color dialog_titlebar_border;
  Color dialog_titlebar_inactive_bg;
  Color disabled_bg;
  Color disabled_border;
  Color disabled_text;
  Color divider;
  Color empty_state_action_hover_bg;
  Color empty_state_color;
  Color empty_state_tag_bg;
  Color empty_state_title_color;
  Color empty_state_watermark_bg;
  Color facts_label_color;
  Color field_description_color;
  Color field_description_emphasis_color;
  Color field_label_color;
  Color focus_border;
  Color ghost_active_background;
  Color ghost_active_foreground;
  Color ghost_foreground;
  Color ghost_hover_background;
  Color ghost_hover_foreground;
  Color ghost_selected_background;
  Color ghost_selected_foreground;
  Color group_title_color;
  Color input_bg;
  Color input_border;
  Color input_border_active;
  Color input_border_hover;
  Color input_foreground;
  Color inset_bg;
  Color keybinding_key_bg;
  Color keybinding_key_border;
  Color keybinding_key_bottom_border;
  Color keybinding_key_fg;
  Color layer_border;
  ShadowList layer_shadow;
  Color link;
  Color list_item_border;
  Color menu_background;
  Color menu_border;
  Color menu_foreground;
  Color menu_selection_background;
  Color menu_selection_foreground;
  ShadowList menu_shadow;
  Color menubar_selection_bg;
  Color notice_surface_bg;
  Color notice_surface_border;
  Color notice_surface_fg;
  ShadowList notice_surface_shadow;
  Color notification_center_empty_color;
  Color notification_center_time_color;
  Color notification_center_title_color;
  Color option_active_bg;
  Color option_hover_bg;
  Color option_selected_active_bg;
  Color option_selected_bg;
  Color option_selected_hover_bg;
  Color panel_subtitle_color;
  Color placeholder_color;
  Color row_list_active_bg;
  Color row_list_hover_bg;
  Color row_list_muted_color;
  Color scrollbar_bg;
  Color scrollbar_shadow;
  Color scrollbar_slider_active_bg;
  Color scrollbar_slider_bg;
  Color scrollbar_slider_hover_bg;
  Color segmented_border;
  Color select_option_description_color;
  Color setting_row_description_color;
  Color slider_track_bg;
  Color state_text_color;
  Color status_bar_bg;
  Color status_bar_inactive_bg;
  Color status_bar_lamp_off;
  Color status_bar_lamp_on;
  Color status_error;
  Color status_success;
  Color status_warning;
  Color switch_list_border;
  Color switch_off_active_bg;
  Color switch_off_bg;
  Color switch_off_hover_bg;
  Color switch_row_color;
  Color switch_thumb_bg;
  ShadowList switch_thumb_shadow;
  Color tab_color;
  Color tab_count_color;
  Color tab_selected_color;
  Color table_border;
  Color table_description_color;
  Color table_header_bg;
  Color table_header_color;
  Color table_row_divider;
  Color table_row_hover;
  Color table_row_hover_solid;
  Color table_row_selected_hover_solid;
  Color table_row_selected_solid;
  Color table_surface;
  Color tabs_steps_color;
  Color text;
  Color text_muted;
  Color title_bar_bg;
  Color title_bar_inactive_bg;
  Color title_bar_subtitle_color;
  Color title_bar_title_color;
  Color toggle_chip_active_border;
  Color toggle_chip_border;
  Color toggle_chip_foreground;
  Color toggle_chip_hover_border;
  Color toggle_chip_hover_foreground;
  Color toggle_chip_on_active_bg;
  Color toggle_chip_on_bg;
  Color toggle_chip_on_border;
  Color toggle_chip_on_foreground;
  Color toggle_chip_on_hover_bg;
  Color toggle_chip_quiet_on_foreground;
  Color toolbar_border;
  Color toolbar_description_color;
  Color tooltip_background;
  Color tooltip_item_icon_color;
  Color tooltip_text_color;
  Color tree_count_color;
  Color tree_drop_target_bg;
};

inline constexpr ThemeTokens dark{
    Color{0xFF297AA0u},
    Color{0xFF256E90u},
    Color{0xFFFFFFFFu},
    Color{0xFF3E87AAu},
    Color{0xFF121314u},
    Color{0xFFDA9F43u},
    Color{0xFFF2B14Au},
    Color{0xFFF3B95Cu},
    Color{0xFF261A05u},
    Color{0xFF8D2823u},
    Color{0xFFA1332Du},
    Color{0xFFB43A33u},
    Color{0xFFFFFFFFu},
    Color{0x0DFFFFFFu},
    Color{0x14FFFFFFu},
    Color{0x1FFFFFFFu},
    Color{0xFFBFBFBFu},
    Color{0xFFBFBFBFu},
    Color{0xFF8C8C8Cu},
    Color{0xFF191A1Bu},
    Color{0xFF232424u},
    ShadowList{0, {}},
    Color{0x38297AA0u},
    Color{0x73297AA0u},
    Color{0xFF8C8C8Cu},
    Color{0xFF0F1011u},
    Color{0xFF111112u},
    Color{0x8CBFBFBFu},
    Color{0x989F9F9Fu},
    Color{0x98CACACAu},
    Color{0xFF28292Au},
    Color{0x14BFBFBFu},
    Color{0xFF191A1Bu},
    Color{0xFF2A2B2Cu},
    Color{0xFF8C8C8Cu},
    Color{0xFFBFBFBFu},
    Color{0xFF121314u},
    Color{0xFF5C1C18u},
    Color{0xFFC42B1Cu},
    Color{0x2EFFE2DFu},
    Color{0xFFFFE2DFu},
    Color{0xFF8D1F14u},
    Color{0xFFC42B1Cu},
    Color{0xFFFFFFFFu},
    Color{0xEB121314u},
    Color{0xEB191A1Bu},
    Color{0xEB121314u},
    ShadowList{1, {Shadow{0.0, 8.0, 32.0, 0.0, Color{0x5C000000u}, false}}},
    Color{0xFF191A1Bu},
    Color{0xFF2A2B2Cu},
    Color{0xFF121314u},
    Color{0x00000000u},
    Color{0x24BFBFBFu},
    Color{0x40BFBFBFu},
    Color{0x14FFFFFFu},
    Color{0x4F5A5D5Eu},
    Color{0xFF8C8C8Cu},
    Color{0x0FBFBFBFu},
    Color{0xFFBFBFBFu},
    Color{0x24BFBFBFu},
    Color{0xFF8C8C8Cu},
    Color{0xFF8C8C8Cu},
    Color{0xFFBFBFBFu},
    kCurrentColor,
    Color{0xFF3994BCu},
    Color{0xFF242526u},
    Color{0xFFFFFFFFu},
    Color{0xFF8C8C8Cu},
    Color{0x33FFFFFFu},
    Color{0xFFBFBFBFu},
    Color{0xFF313233u},
    Color{0xFFFFFFFFu},
    Color{0xFF8C8C8Cu},
    Color{0xFF111112u},
    Color{0xFF424344u},
    Color{0xFF3C3C3Du},
    Color{0xFF555657u},
    Color{0xFFBFBFBFu},
    Color{0xFF0F1011u},
    Color{0x2B808080u},
    Color{0x99333333u},
    Color{0x99444444u},
    Color{0xFFCCCCCCu},
    Color{0xFF38393Au},
    ShadowList{1, {Shadow{0.0, 4.0, 20.0, 0.0, Color{0x4D000000u}, false}}},
    Color{0xFF48A0C7u},
    Color{0x1FBFBFBFu},
    Color{0xEB191A1Bu},
    Color{0xFF333536u},
    Color{0xFFBFBFBFu},
    Color{0x1FFFFFFFu},
    Color{0xFFFFFFFFu},
    ShadowList{1, {Shadow{0.0, 8.0, 32.0, 0.0, Color{0x5C000000u}, false}}},
    Color{0xFF242526u},
    Color{0xEB191A1Bu},
    Color{0xFF333536u},
    Color{0xFFBFBFBFu},
    ShadowList{1, {Shadow{0.0, 8.0, 32.0, 0.0, Color{0x5C000000u}, false}}},
    Color{0xFF8C8C8Cu},
    Color{0xFF8C8C8Cu},
    Color{0xFF8C8C8Cu},
    Color{0x38000000u},
    Color{0x17FFFFFFu},
    Color{0x4D256E90u},
    Color{0x4D297AA0u},
    Color{0x4D3E87AAu},
    Color{0xFF8C8C8Cu},
    Color{0xFF8C8C8Cu},
    Color{0x14BFBFBFu},
    Color{0x0DBFBFBFu},
    Color{0xFF8C8C8Cu},
    Color{0x00000000u},
    Color{0xFF000000u},
    Color{0x66BFBFBFu},
    Color{0x66797979u},
    Color{0xB3646464u},
    Color{0x47BFBFBFu},
    Color{0xFF8C8C8Cu},
    Color{0xFF8C8C8Cu},
    Color{0x33BFBFBFu},
    Color{0xFF8C8C8Cu},
    Color{0xFF191A1Bu},
    Color{0xFF121314u},
    Color{0xFF4E545Cu},
    Color{0xFF4FDC8Au},
    Color{0xFFFF7B72u},
    Color{0xFF4FDC8Au},
    Color{0xFFF2B14Au},
    Color{0x47BFBFBFu},
    Color{0x477B7B7Bu},
    Color{0x33BFBFBFu},
    Color{0x47D6D6D6u},
    kCurrentColor,
    Color{0xFFFFFFFFu},
    ShadowList{1, {Shadow{0.0, 1.0, 3.0, 0.0, Color{0x38000000u}, false}}},
    Color{0xFF8C8C8Cu},
    kCurrentColor,
    Color{0xFFBFBFBFu},
    Color{0x1FBFBFBFu},
    Color{0xFF8C8C8Cu},
    Color{0xFF1C1D1Eu},
    Color{0xFF8C8C8Cu},
    Color{0x14BFBFBFu},
    Color{0x0FBFBFBFu},
    Color{0xFF212223u},
    Color{0xFF233A44u},
    Color{0xFF1D3642u},
    Color{0xFF171819u},
    Color{0xFFBFBFBFu},
    Color{0xFFBFBFBFu},
    Color{0xFF8C8C8Cu},
    Color{0xFF191A1Bu},
    Color{0xFF121314u},
    Color{0xFF8C8C8Cu},
    Color{0xFFBFBFBFu},
    Color{0xFF3C3C3Du},
    Color{0xFF424344u},
    Color{0xFF8C8C8Cu},
    Color{0xFF555657u},
    Color{0xFFBFBFBFu},
    Color{0x38256E90u},
    Color{0x38297AA0u},
    Color{0x99297AA0u},
    Color{0xFFBFBFBFu},
    Color{0x473E87AAu},
    Color{0xFF48A0C7u},
    Color{0x14FFFFFFu},
    Color{0xFF8C8C8Cu},
    Color{0xEB191A1Bu},
    Color{0xFF8C8C8Cu},
    Color{0xFF8C8C8Cu},
    Color{0xFF8C8C8Cu},
    Color{0x1F3994BCu},
};

inline constexpr ThemeTokens light{
    Color{0xFF0069CCu},
    Color{0xFF005FB8u},
    Color{0xFFFFFFFFu},
    Color{0xFF1A78D1u},
    Color{0xFFFFFFFFu},
    Color{0xFF985F0Eu},
    Color{0xFFA96A10u},
    Color{0xFFB27928u},
    Color{0xFFFFFFFFu},
    Color{0xFF962B25u},
    Color{0xFFA93029u},
    Color{0xFFBA362Eu},
    Color{0xFFFFFFFFu},
    Color{0x335A5A5Au},
    Color{0x245A5A5Au},
    Color{0x1A5A5A5Au},
    Color{0xFF2F2F2Fu},
    Color{0xFF202020u},
    Color{0xFF606060u},
    Color{0xFFF4F4F4u},
    Color{0xFFC7C7C7u},
    ShadowList{1, {Shadow{0.0, 1.0, 2.0, 0.0, Color{0x12000000u}, false}}},
    Color{0x380069CCu},
    Color{0x730069CCu},
    Color{0xFF606060u},
    Color{0xFFE3E3E3u},
    Color{0xFFFCFCFCu},
    Color{0x99202020u},
    Color{0xA31B1B1Bu},
    Color{0xA3434343u},
    Color{0xFFFDFDFDu},
    Color{0x14202020u},
    Color{0xFFFAFAFDu},
    Color{0xFFF0F1F2u},
    Color{0xFF606060u},
    Color{0xFF202020u},
    Color{0xFFFFFFFFu},
    Color{0xFFFDE8E6u},
    Color{0xFFC42B1Cu},
    Color{0x2E78140Eu},
    Color{0xFF78140Eu},
    Color{0xFF8D1F14u},
    Color{0xFFC42B1Cu},
    Color{0xFFFFFFFFu},
    Color{0xEBFFFFFFu},
    Color{0xEBFAFAFDu},
    Color{0xEBFAFAFDu},
    ShadowList{1, {Shadow{0.0, 8.0, 32.0, 0.0, Color{0x29000000u}, false}}},
    Color{0xFFFAFAFDu},
    Color{0xFFF0F1F2u},
    Color{0xFFFAFAFDu},
    Color{0x00000000u},
    Color{0x24202020u},
    Color{0x40202020u},
    Color{0x14000000u},
    Color{0x1F000000u},
    Color{0xFF606060u},
    Color{0x0F202020u},
    Color{0xFF202020u},
    Color{0x24202020u},
    Color{0xFF606060u},
    Color{0xFF606060u},
    Color{0xFF202020u},
    kCurrentColor,
    Color{0xFF0069CCu},
    Color{0xFFEAEAEAu},
    Color{0xFF000000u},
    Color{0xFF606060u},
    Color{0xFFD6D6D8u},
    Color{0xFF202020u},
    Color{0xFFD6D6D6u},
    Color{0xFF000000u},
    Color{0xFF606060u},
    Color{0xFFFCFCFCu},
    Color{0xFFB3B3B3u},
    Color{0xFFA1A1A1u},
    Color{0xFFBBBBBBu},
    Color{0xFF202020u},
    Color{0xFFF6F6F6u},
    Color{0x66DDDDDDu},
    Color{0x66CCCCCCu},
    Color{0x66BBBBBBu},
    Color{0xFF555555u},
    Color{0xFFC7C7C7u},
    ShadowList{1, {Shadow{0.0, 4.0, 20.0, 0.0, Color{0x4D000000u}, false}}},
    Color{0xFF0069CCu},
    Color{0x1F202020u},
    Color{0xEBFAFAFDu},
    Color{0xFFD8D8D8u},
    Color{0xFF202020u},
    Color{0x14000000u},
    Color{0xFF202020u},
    ShadowList{1, {Shadow{0.0, 8.0, 32.0, 0.0, Color{0x29000000u}, false}}},
    Color{0xFFEAEAEAu},
    Color{0xEBFAFAFDu},
    Color{0xFFD8D8D8u},
    Color{0xFF202020u},
    ShadowList{1, {Shadow{0.0, 8.0, 32.0, 0.0, Color{0x29000000u}, false}}},
    Color{0xFF606060u},
    Color{0xFF606060u},
    Color{0xFF606060u},
    Color{0x2E202020u},
    Color{0x17202020u},
    Color{0x4D005FB8u},
    Color{0x4D0069CCu},
    Color{0x4D1A78D1u},
    Color{0xFF606060u},
    Color{0xFF606060u},
    Color{0x14202020u},
    Color{0x0D202020u},
    Color{0xFF606060u},
    Color{0x00000000u},
    Color{0xFFDDDDDDu},
    Color{0x99000000u},
    Color{0x66646464u},
    Color{0xB3646464u},
    Color{0x47202020u},
    Color{0xFF606060u},
    Color{0xFF606060u},
    Color{0x33202020u},
    Color{0xFF606060u},
    Color{0xFFFAFAFDu},
    Color{0xFFFAFAFDu},
    Color{0xFFBEC2C8u},
    Color{0xFF1FA050u},
    Color{0xFFB42318u},
    Color{0xFF1F7A3Fu},
    Color{0xFFA96A10u},
    Color{0x47202020u},
    Color{0x47151515u},
    Color{0x33202020u},
    Color{0x47707070u},
    kCurrentColor,
    Color{0xFFFFFFFFu},
    ShadowList{1, {Shadow{0.0, 1.0, 3.0, 0.0, Color{0x38000000u}, false}}},
    Color{0xFF606060u},
    kCurrentColor,
    Color{0xFF202020u},
    Color{0x1F202020u},
    Color{0xFF606060u},
    Color{0xFFF2F2F2u},
    Color{0xFF606060u},
    Color{0x14202020u},
    Color{0x0F202020u},
    Color{0xFFEBEBEBu},
    Color{0xFFB5D2EDu},
    Color{0xFFAECDEBu},
    Color{0xFFF8F8F8u},
    Color{0xFF202020u},
    Color{0xFF202020u},
    Color{0xFF606060u},
    Color{0xFFFAFAFDu},
    Color{0xFFFAFAFDu},
    Color{0xFF606060u},
    Color{0xFF202020u},
    Color{0xFFA1A1A1u},
    Color{0xFFB3B3B3u},
    Color{0xFF606060u},
    Color{0xFFBBBBBBu},
    Color{0xFF202020u},
    Color{0x38005FB8u},
    Color{0x380069CCu},
    Color{0x990069CCu},
    Color{0xFF202020u},
    Color{0x471A78D1u},
    Color{0xFF0069CCu},
    Color{0x14000000u},
    Color{0xFF606060u},
    Color{0xEBFAFAFDu},
    Color{0xFF606060u},
    Color{0xFF606060u},
    Color{0xFF606060u},
    Color{0x1F0069CCu},
};

}  // namespace rujian::ui::tokens

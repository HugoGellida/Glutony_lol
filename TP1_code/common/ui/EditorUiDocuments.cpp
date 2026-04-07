#include "EditorUiDocuments.hpp"

namespace
{
const char* kEditorLayoutDocument = R"RML(
<rml>
<head>
    <style>
        body {
            margin: 0px;
            color: #c7d0d9;
            font-family: LatoLatin;
            font-size: 14px;
            overflow: hidden;
        }

        #root {
            position: absolute;
            left: 0px;
            top: 0px;
            width: 100%;
            height: 100%;
            overflow: hidden;
        }

        #builder_header {
            position: absolute;
            left: 0px;
            top: 0px;
            width: 100%;
            height: 36px;
            background-color: #151b21;
            border-bottom: 1px #2a353e;
            overflow: visible;
        }

        .builder_menu_bar {
            height: 100%;
            display: flex;
            align-items: stretch;
        }

        .builder_menu {
            position: relative;
            height: 100%;
            overflow: visible;
        }

        .builder_menu_button {
            height: 100%;
            min-width: 60px;
            padding: 0px 14px;
            line-height: 36px;
            color: #d6dee5;
            background-color: #151b21;
        }

        .builder_menu_button:hover {
            background-color: #202933;
        }

        .builder_menu_dropdown {
            position: absolute;
            left: 0px;
            top: 36px;
            display: block;
            min-width: 140px;
            background-color: #1c242b;
            border: 1px #2f3b45;
            z-index: 20;
        }

        .builder_menu_item {
            display: block;
            width: 100%;
            padding: 10px 14px;
            color: #d6dee5;
            border-bottom: 1px #2a353e;
        }

        .builder_menu_item:last-child {
            border-bottom: 0px;
        }

        .builder_menu_item:hover {
            background-color: #27323b;
        }

        .panel {
            position: absolute;
            background-color: #171c21;
            overflow: hidden;
        }

        .panel_nested {
            left: 0px;
            width: 100%;
        }

        .panel_shell {
            width: 100%;
            height: 100%;
            display: flex;
            flex-direction: column;
            overflow: hidden;
        }

        .inspector_shell {
            position: relative;
        }

        .inspector_overlay {
            position: absolute;
            left: 0px;
            top: 0px;
            width: 100%;
            height: 100%;
            overflow: visible;
            pointer-events: none;
        }

        .inspector_overlay .hierarchy_context_menu {
            pointer-events: auto;
        }

        .hierarchy_shell {
            position: relative;
        }

        .panel_header {
            display: block;
            position: relative;
            z-index: 1;
            flex-shrink: 0;
            width: 100%;
            height: 34px;
            line-height: 34px;
            padding-left: 12px;
            background-color: #20272e;
            border-bottom: 1px #2f3b45;
            font-size: 12px;
            letter-spacing: 1.2px;
            text-transform: uppercase;
            color: #8ea0b0;
            box-sizing: border-box;
        }

        .panel_header_with_action {
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding-right: 8px;
            box-sizing: border-box;
        }

        .panel_header_tabs {
            display: flex;
            align-items: center;
            justify-content: space-between;
            width: 100%;
            padding-right: 8px;
            box-sizing: border-box;
        }

        .panel_tabs {
            display: flex;
            align-items: center;
        }

        .panel_tab_button {
            min-width: 110px;
            margin-right: 8px;
            padding: 0px 10px;
            color: #94a6b4;
            background-color: #151d23;
            border: 1px #2a3640;
            text-align: center;
        }

        .panel_tab_button.active {
            color: #dce5ec;
            background-color: #25323b;
            border-color: #566f81;
        }

        .panel_tab_button:hover {
            background-color: #202b33;
        }

        .panel_header_action {
            min-width: 22px;
            height: 22px;
            line-height: 22px;
            margin-top: 6px;
            text-align: center;
            color: #dbe4ea;
            background-color: #27323b;
            border: 1px #3a4a56;
        }

        .panel_header_action:hover {
            background-color: #31404a;
        }

        .panel_body {
            flex: 1;
            min-height: 0px;
            padding: 12px;
            overflow: auto;
            scrollbar-margin: 12px;
        }

        .panel_shell > div[data-ui-slot='content'].panel_body scrollbarvertical {
            width: 12px;
            min-width: 12px;
            max-width: 12px;
        }

        .panel_shell > div[data-ui-slot='content'].panel_body scrollbarvertical slidertrack {
            background-color: #141b21;
            border-left: 1px #25323b;
        }

        .panel_shell > div[data-ui-slot='content'].panel_body scrollbarvertical sliderbar {
            width: 12px;
            min-width: 12px;
            max-width: 12px;
            min-height: 28px;
            margin-left: 1px;
            background-color: #4a6273;
        }

        .panel_shell > div[data-ui-slot='content'].panel_body scrollbarvertical sliderbar:hover {
            background-color: #6f8a9c;
        }

        .panel_body_no_padding {
            padding: 0px;
            overflow: hidden;
            position: relative;
        }

        .hierarchy_body,
        .widget_catalog_body {
            scrollbar-margin: 12px;
            overflow-x: hidden;
            overflow-y: auto;
        }

        .hierarchy_body scrollbarvertical,
        .widget_catalog_body scrollbarvertical {
            width: 12px;
        }

        .hierarchy_body scrollbarvertical slidertrack,
        .widget_catalog_body scrollbarvertical slidertrack {
            background-color: #141b21;
            border-left: 1px #25323b;
        }

        .hierarchy_body scrollbarvertical sliderbar,
        .widget_catalog_body scrollbarvertical sliderbar {
            width: 12px;
            min-height: 28px;
            margin-left: 1px;
            background-color: #4a6273;
        }

        .hierarchy_body scrollbarvertical sliderbar:hover,
        .widget_catalog_body scrollbarvertical sliderbar:hover {
            background-color: #6f8a9c;
        }

        .placeholder_block {
            display: block;
            width: 100%;
            margin-bottom: 12px;
            padding: 10px 12px;
            background-color: #1e252c;
            border: 1px #2b3740;
            box-sizing: border-box;
        }

        .placeholder_title {
            display: block;
            width: 100%;
            margin-bottom: 6px;
            font-size: 13px;
            color: #d7e0e8;
            line-height: 16px;
            white-space: normal;
            box-sizing: border-box;
        }

        .placeholder_text {
            display: block;
            width: 100%;
            font-size: 12px;
            color: #7f919f;
            line-height: 16px;
            white-space: normal;
            box-sizing: border-box;
        }

        .inspector_panel_body {
            padding: 10px;
            scrollbar-margin: 12px;
            overflow-x: hidden;
            overflow-y: auto;
        }

        .inspector_panel_body scrollbarvertical {
            width: 12px;
        }

        .inspector_panel_body scrollbarvertical slidertrack {
            background-color: #141b21;
            border-left: 1px #25323b;
        }

        .inspector_panel_body scrollbarvertical sliderbar {
            width: 12px;
            min-height: 28px;
            margin-left: 1px;
            background-color: #4a6273;
        }

        .inspector_panel_body scrollbarvertical sliderbar:hover {
            background-color: #6f8a9c;
        }

        .inspector_summary {
            display: block;
            width: 100%;
            margin-bottom: 10px;
            padding: 10px 12px;
            background-color: #1e252c;
            border: 1px #2b3740;
            box-sizing: border-box;
        }

        .inspector_summary_title {
            display: block;
            width: 100%;
            margin-bottom: 4px;
            color: #d7e0e8;
            font-size: 13px;
            line-height: 16px;
            white-space: normal;
            box-sizing: border-box;
        }

        .inspector_summary_text {
            display: block;
            width: 100%;
            color: #7f919f;
            font-size: 12px;
            line-height: 16px;
            white-space: normal;
            box-sizing: border-box;
        }

        .inspector_section,
        .inspector_foldout {
            margin-bottom: 10px;
            background-color: #1b2329;
            border: 1px #2c3943;
        }

        .inspector_field_row {
            display: flex;
            flex-wrap: wrap;
            align-items: center;
            width: 100%;
            padding: 8px 10px;
            border-bottom: 1px #27323b;
            box-sizing: border-box;
        }

        .inspector_field_row:last-child {
            border-bottom: 0px;
        }

        .inspector_field_name {
            width: 40%;
            padding-right: 10px;
            color: #cfd9e2;
            font-size: 12px;
        }

        .inspector_field_input {
            width: 60%;
            height: 30px;
            padding: 0px 8px;
            background-color: #11181d;
            color: #e3ebf2;
            border: 1px #33424d;
            box-sizing: border-box;
            font-size: 12px;
        }

        .inspector_asset_field {
            border-color: #48606f;
            background-color: #121b21;
        }

        .inspector_asset_field_active {
            border-color: #9fc1dd;
            background-color: #1d2d37;
        }

        .inspector_asset_hint {
            width: 60%;
            margin-left: 40%;
            margin-top: 4px;
            color: #7f919f;
            font-size: 10px;
            text-transform: uppercase;
        }

        .inspector_add_component_row {
            display: flex;
            justify-content: flex-end;
            margin-top: 12px;
        }

        .inspector_add_component_button {
            min-width: 132px;
            margin-top: 0px;
            text-transform: uppercase;
            font-size: 11px;
            letter-spacing: 0.8px;
        }

        .inspector_add_component_menu {
            z-index: 70;
        }

        .inspector_component_context_menu {
            z-index: 70;
        }

        .inspector_field_input option {
            background-color: #11181d;
            color: #e3ebf2;
        }

        .inspector_field_input value,
        .inspector_field_input selectbox,
        .inspector_field_input selectbox option,
        .inspector_field_input selectbox option:hover,
        .inspector_field_input selectbox option:checked {
            background-color: #11181d;
            color: #e3ebf2;
            border: 1px #33424d;
        }

        .inspector_foldout_header {
            display: flex;
            align-items: center;
            padding: 9px 10px;
            background-color: #202a32;
            border-bottom: 1px #2c3943;
        }

        .inspector_foldout_header:hover {
            background-color: #27323b;
        }

        .inspector_foldout_icon {
            width: 18px;
            color: #8ea0b0;
            font-size: 12px;
        }

        .inspector_foldout_title {
            color: #d7e0e8;
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.8px;
        }

        .inspector_foldout_body {
            display: block;
        }

        .hierarchy_body,
        .widget_catalog_body {
            padding: 8px;
            display: block;
        }

        .hierarchy_node {
            display: block;
            width: 100%;
            margin-bottom: 2px;
            box-sizing: border-box;
        }

        .hierarchy_children {
            display: block;
            margin-left: 14px;
        }

        .hierarchy_row {
            display: block;
            min-height: 34px;
            padding: 7px 10px;
            background-color: #1e252c;
            border: 1px #2c3943;
            box-sizing: border-box;
            width: 100%;
            white-space: normal;
            drag: drag-drop;
        }

        .scene_hierarchy_row {
            drag: none;
        }

        .hierarchy_row:hover {
            background-color: #263039;
            border-color: #466170;
        }

        .hierarchy_row.selected {
            background-color: #243643;
            border-color: #5f7b90;
        }

        .hierarchy_row.dragging {
            opacity: 0.55;
        }

        .hierarchy_row.drop_active {
            border-color: #92b6d4;
            background-color: #2a4150;
        }

        .hierarchy_label {
            display: block;
            color: #dde5eb;
            font-size: 13px;
            white-space: normal;
        }

        .hierarchy_meta {
            display: block;
            margin-top: 3px;
            font-size: 11px;
            color: #8ea0b0;
            text-transform: uppercase;
            white-space: normal;
        }

        .hierarchy_drop_zone {
            height: 8px;
            margin: 2px 0px;
            background-color: transparent;
        }

        .hierarchy_drop_zone:hover {
            background-color: #496274;
        }

        .hierarchy_drop_zone.drop_active {
            background-color: #8bb1d0;
        }

        .hierarchy_context_menu {
            position: absolute;
            min-width: 144px;
            max-height: 320px;
            background-color: #1b2329;
            border: 1px #33414c;
            z-index: 40;
            overflow-x: hidden;
            overflow-y: auto;
        }

        .hierarchy_context_item {
            display: block;
            padding: 9px 12px;
            color: #dbe4ea;
            border-bottom: 1px #2a353e;
        }

        .hierarchy_context_item:last-child {
            border-bottom: 0px;
        }

        .hierarchy_context_item:hover {
            background-color: #27323b;
        }

        .hierarchy_context_item.disabled {
            color: #667784;
        }

        .hierarchy_context_item.disabled:hover {
            background-color: transparent;
        }

        .hierarchy_context_item.danger {
            color: #efb0b0;
        }

        .widget_catalog_item {
            display: block;
            width: 100%;
            margin-bottom: 8px;
            padding: 10px 12px;
            background-color: #1e252c;
            border: 1px #2c3943;
            box-sizing: border-box;
            white-space: normal;
            drag: clone;
        }

        .widget_catalog_item:hover {
            background-color: #263039;
            border-color: #3f5260;
        }

        .widget_catalog_label {
            display: block;
            margin-bottom: 4px;
            color: #dde5eb;
            font-size: 13px;
            white-space: normal;
        }

        .widget_catalog_text {
            display: block;
            font-size: 12px;
            color: #8ea0b0;
            white-space: normal;
        }

        .preview_shell {
            width: 100%;
            height: 100%;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: flex-start;
            background-color: #11171c;
            padding: 18px;
            box-sizing: border-box;
        }

        .preview_toolbar {
            width: 100%;
            margin-bottom: 12px;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }

        .preview_toolbar_group {
            display: flex;
            align-items: center;
        }

        .preview_toolbar_button {
            min-width: 34px;
            height: 30px;
            margin-right: 8px;
            padding: 0px 10px;
            line-height: 30px;
            text-align: center;
            color: #dce5ec;
            background-color: #1b252d;
            border: 1px #31404b;
        }

        .preview_toolbar_button:hover {
            background-color: #25323b;
        }

        .preview_zoom_label {
            min-width: 62px;
            height: 30px;
            margin-right: 8px;
            line-height: 30px;
            text-align: center;
            color: #b6c4cf;
            background-color: #141b21;
            border: 1px #29353f;
        }

        .preview_canvas {
            width: 100%;
            flex: 1;
            position: relative;
            display: block;
            overflow: hidden;
            padding: 16px;
            background-color: #0d1318;
            border: 1px #28333c;
            box-sizing: border-box;
        }

        .scene_viewport_shell {
            width: 100%;
            height: 100%;
            position: relative;
            display: flex;
            flex-direction: column;
            padding: 12px;
            background-color: transparent;
            box-sizing: border-box;
        }

        .scene_viewport_toolbar {
            width: 100%;
            margin-bottom: 10px;
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 10px;
            background-color: #11171c;
            border: 1px #28333c;
            box-sizing: border-box;
        }

        .scene_playback_status {
            min-width: 132px;
            height: 30px;
            padding: 0px 10px;
            line-height: 30px;
            text-align: center;
            color: #b6c4cf;
            background-color: #141b21;
            border: 1px #29353f;
        }

        .scene_document_status {
            max-width: 360px;
            margin-right: 10px;
            padding: 0px 10px;
            line-height: 30px;
            color: #94a6b4;
            background-color: #141b21;
            border: 1px #29353f;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
        }

        .scene_viewport_surface {
            width: 100%;
            flex: 1;
            display: block;
            background-color: transparent;
            border: 0px;
            box-sizing: border-box;
        }

        .scene_modal_overlay {
            position: absolute;
            left: 0px;
            top: 0px;
            right: 0px;
            bottom: 0px;
            display: flex;
            align-items: center;
            justify-content: center;
            background-color: rgba(8, 12, 15, 0.74);
        }

        .scene_modal_card {
            width: 320px;
            padding: 16px;
            background-color: #171f25;
            border: 1px #33424d;
        }

        .scene_modal_title {
            margin-bottom: 8px;
            color: #e2eaf0;
            font-size: 15px;
        }

        .scene_modal_text {
            margin-bottom: 14px;
            color: #8ea0b0;
            font-size: 12px;
            white-space: normal;
        }

        .scene_modal_actions {
            display: flex;
            justify-content: flex-end;
        }

        .scene_modal_button {
            margin-left: 8px;
            padding: 8px 12px;
            color: #dbe4ea;
            background-color: #24303a;
            border: 1px #364651;
        }

        .scene_modal_button:hover {
            background-color: #2c3943;
        }

        .scene_modal_button.primary {
            background-color: #2d4350;
            border-color: #58778a;
        }

        .scene_modal_button.danger {
            color: #efb0b0;
        }

        .preview_window {
            position: absolute;
            display: block;
            overflow: hidden;
            background-color: #ffffff;
            border: 2px #718391;
        }

        .preview_host {
            position: absolute;
            left: 0px;
            top: 0px;
            right: 0px;
            bottom: 0px;
            overflow: hidden;
            background-color: #ffffff;
        }

        .preview_resize_handle {
            position: absolute;
            background-color: rgba(63, 84, 100, 0.14);
            z-index: 4;
        }

        .preview_resize_handle:hover {
            background-color: rgba(95, 126, 150, 0.34);
        }

        .preview_resize_right {
            top: 0px;
            right: 0px;
            width: 10px;
            bottom: 0px;
            cursor: ew-resize;
        }

        .preview_resize_bottom {
            left: 0px;
            bottom: 0px;
            right: 0px;
            height: 10px;
            cursor: ns-resize;
        }

        .preview_resize_corner {
            right: 0px;
            bottom: 0px;
            width: 14px;
            height: 14px;
            cursor: nwse-resize;
            background-color: rgba(20, 28, 34, 0.22);
        }

        .splitter {
            position: absolute;
            background-color: #28313a;
            border: 0px;
            cursor: resize;
        }

        .splitter:hover {
            background-color: #4a6578;
        }

        .splitter_horizontal_nested {
            cursor: ns-resize;
        }

        .splitter_vertical_nested {
            cursor: ew-resize;
        }

        .asset_browser_workspace {
            position: relative;
            width: 100%;
            height: 100%;
            background-color: #11171c;
        }

        .asset_browser_overlay {
            position: absolute;
            left: 0px;
            top: 0px;
            width: 100%;
            height: 100%;
            overflow: visible;
            pointer-events: none;
            z-index: 25;
        }

        .asset_browser_overlay .hierarchy_context_menu {
            pointer-events: auto;
        }

        .console_workspace {
            display: block;
            position: relative;
            width: 100%;
            height: 100%;
            min-width: 0px;
            min-height: 0px;
            overflow: hidden;
            background-color: #0f151a;
        }

        .console_output {
            display: block;
            position: relative;
            width: 100%;
            height: 100%;
            min-width: 0px;
            min-height: 0px;
            padding: 10px 18px 10px 12px;
            scrollbar-margin: 14px;
            overflow-y: auto;
            overflow-x: hidden;
            background-color: #0f151a;
            box-sizing: border-box;
        }

        .console_output scrollbarvertical {
            display: block;
            flex: 0 0 12px;
            width: 12px;
            max-width: 12px;
            min-width: 12px;
        }

        .console_output scrollbarvertical slidertrack {
            display: block;
            width: 12px;
            max-width: 12px;
            min-width: 12px;
            background-color: #141b21;
            border-left: 1px #25323b;
        }

        .console_output scrollbarvertical sliderbar {
            display: block;
            width: 12px;
            max-width: 12px;
            min-width: 12px;
            min-height: 28px;
            margin-left: 1px;
            background-color: #4a6273;
        }

        .console_output scrollbarvertical sliderbar:hover {
            background-color: #6f8a9c;
        }

        .console_line {
            display: block;
            margin-bottom: 4px;
            padding: 2px 0px;
            font-size: 12px;
            color: #d8e1e8;
            white-space: pre-wrap;
            word-break: break-word;
        }

        .console_line_segment {
            color: #d8e1e8;
            white-space: pre-wrap;
        }

        .console_line_bold {
            font-weight: bold;
        }

        .console_line_info {
            color: #8fc4ff;
        }

        .console_line_warning {
            color: #f0c56d;
        }

        .console_line_error {
            color: #f08d8d;
        }

        .console_line_success {
            color: #80d69e;
        }

        .console_line_neutral {
            color: #d8e1e8;
        }

        .console_ansi_black {
            color: #798892;
        }

        .console_ansi_red {
            color: #f08d8d;
        }

        .console_ansi_green {
            color: #80d69e;
        }

        .console_ansi_yellow {
            color: #f0c56d;
        }

        .console_ansi_blue {
            color: #7fbfff;
        }

        .console_ansi_magenta {
            color: #d1a1f3;
        }

        .console_ansi_cyan {
            color: #83d7dc;
        }

        .console_ansi_white {
            color: #f1f6fb;
        }

        .asset_browser_pane {
            position: absolute;
            top: 0px;
            bottom: 0px;
            display: flex;
            flex-direction: column;
            min-width: 0px;
            min-height: 0px;
            box-sizing: border-box;
            overflow: hidden;
        }

        .asset_browser_files_pane {
            background-color: #13191e;
        }

        .asset_browser_tree_pane {
            background-color: #171e24;
            border-right: 1px #2a353e;
        }

        .asset_browser_section_header {
            display: block;
            flex: 0 0 auto;
            min-height: 34px;
            padding: 10px 12px;
            background-color: #1c242b;
            border-bottom: 1px #2d3943;
            color: #d6dee5;
            font-size: 12px;
            letter-spacing: 0.9px;
            text-transform: uppercase;
            box-sizing: border-box;
        }

        .asset_browser_section_path {
            display: block;
            margin-top: 3px;
            color: #879aa8;
            font-size: 11px;
            letter-spacing: 0px;
            text-transform: none;
        }

        .asset_browser_section_body {
            flex: 1 1 auto;
            height: 0px;
            min-height: 0px;
            padding: 10px;
            overflow-x: hidden;
            overflow-y: auto;
            box-sizing: border-box;
        }

        .asset_browser_tree_body scrollbarvertical,
        .asset_browser_files_body scrollbarvertical {
            width: 12px;
        }

        .asset_browser_tree_body scrollbarvertical slidertrack,
        .asset_browser_files_body scrollbarvertical slidertrack {
            background-color: #141b21;
            border-left: 1px #25323b;
        }

        .asset_browser_tree_body scrollbarvertical sliderbar,
        .asset_browser_files_body scrollbarvertical sliderbar {
            width: 12px;
            min-height: 28px;
            margin-left: 1px;
            background-color: #4a6273;
        }

        .asset_browser_tree_node {
            display: block;
            width: 100%;
            margin-bottom: 2px;
        }

        .asset_browser_tree_children {
            display: block;
            margin-left: 16px;
        }

        .asset_browser_tree_row {
            display: flex;
            align-items: center;
            min-height: 32px;
            padding: 6px 8px;
            background-color: #1c252c;
            border: 1px #2b3740;
            box-sizing: border-box;
        }

        .asset_browser_tree_row:hover {
            background-color: #243039;
            border-color: #415563;
        }

        .asset_browser_tree_row.selected {
            background-color: #29404f;
            border-color: #67839a;
        }

        .asset_browser_tree_toggle {
            width: 18px;
            color: #8ea0b0;
            font-size: 11px;
            text-align: center;
        }

        .asset_browser_tree_label {
            flex: 1;
            color: #dce5eb;
            font-size: 12px;
            white-space: normal;
        }

        .asset_browser_tree_meta {
            color: #7f919f;
            font-size: 10px;
            text-transform: uppercase;
        }

        .asset_browser_file_grid {
            display: flex;
            flex-wrap: wrap;
            align-content: flex-start;
        }

        .asset_browser_file_card {
            width: 168px;
            min-height: 112px;
            margin-right: 10px;
            margin-bottom: 10px;
            padding: 10px 12px;
            background-color: #1d252c;
            border: 1px #2b3740;
            box-sizing: border-box;
            drag: clone;
        }

        .asset_browser_file_card:hover {
            background-color: #263039;
            border-color: #466170;
        }

        .asset_browser_file_card.selected {
            background-color: #29404f;
            border-color: #66859c;
        }

        .asset_browser_file_card.dragging {
            opacity: 0.55;
        }

        .asset_browser_file_badge {
            display: inline-block;
            margin-bottom: 10px;
            padding: 2px 7px;
            background-color: #10161b;
            border: 1px #34424e;
            color: #a7bac8;
            font-size: 10px;
            text-transform: uppercase;
        }

        .asset_browser_file_card.material .asset_browser_file_badge {
            color: #f2c58d;
            border-color: #6b4f23;
        }

        .asset_browser_file_card.mesh .asset_browser_file_badge {
            color: #8fd6d6;
            border-color: #2e6262;
        }

        .asset_browser_file_card.shader .asset_browser_file_badge {
            color: #d0a6ef;
            border-color: #5b3b73;
        }

        .asset_browser_file_card.texture .asset_browser_file_badge {
            color: #92d39e;
            border-color: #345b3b;
        }

        .asset_browser_file_card.scene .asset_browser_file_badge {
            color: #d9cf8c;
            border-color: #62572a;
        }

        .asset_browser_file_label {
            display: block;
            margin-bottom: 8px;
            color: #dde5eb;
            font-size: 13px;
            white-space: normal;
        }

        .asset_browser_file_meta {
            display: block;
            color: #7f919f;
            font-size: 11px;
            white-space: normal;
            line-height: 15px;
        }

        .asset_browser_context_menu {
            z-index: 60;
        }

        #center_panel {
            position: absolute;
            overflow: hidden;
        }

        #viewport_panel {
            position: absolute;
            overflow: hidden;
            background-color: transparent;
        }

        #bottom_panel {
            position: absolute;
            background-color: #151a1f;
            overflow: hidden;
        }
    </style>
</head>
<body>
    <div id="root">
        <div id="builder_header">
            <div class="builder_menu_bar">
                <div id="builder_menu_file" class="builder_menu">
                    <div id="builder_menu_file_button" class="builder_menu_button">File</div>
                    <div id="builder_menu_file_dropdown" class="builder_menu_dropdown">
                        <div id="builder_menu_new" class="builder_menu_item">New</div>
                        <div id="builder_menu_open" class="builder_menu_item">Open</div>
                        <div id="builder_menu_save_as" class="builder_menu_item">Save As</div>
                        <div id="builder_menu_back_to_editor" class="builder_menu_item">Back to editor</div>
                    </div>
                </div>
                <div id="builder_menu_window" class="builder_menu">
                    <div id="builder_menu_window_button" class="builder_menu_button">Window</div>
                    <div id="builder_menu_window_dropdown" class="builder_menu_dropdown">
                        <div id="builder_menu_open_ui_builder" class="builder_menu_item">UI Builder</div>
                    </div>
                </div>
            </div>
        </div>
        <div id="left_panel" class="panel"></div>
        <div id="left_splitter" class="splitter"></div>
        <div id="center_panel">
            <div id="viewport_panel"></div>
            <div id="horizontal_splitter" class="splitter"></div>
            <div id="bottom_panel"></div>
        </div>
        <div id="right_splitter" class="splitter"></div>
        <div id="right_panel" class="panel"></div>
    </div>
</body>
</rml>
)RML";
}

const char* getEditorLayoutDocument()
{
    return kEditorLayoutDocument;
}
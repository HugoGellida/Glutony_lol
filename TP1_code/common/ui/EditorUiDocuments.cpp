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
        }

        .hierarchy_shell {
            position: relative;
        }

        .panel_header {
            height: 34px;
            line-height: 34px;
            padding-left: 12px;
            background-color: #20272e;
            border-bottom: 1px #2f3b45;
            font-size: 12px;
            letter-spacing: 1.2px;
            text-transform: uppercase;
            color: #8ea0b0;
        }

        .panel_body {
            flex: 1;
            padding: 12px;
            overflow: auto;
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
            margin-bottom: 12px;
            padding: 10px 12px;
            background-color: #1e252c;
            border: 1px #2b3740;
        }

        .placeholder_title {
            margin-bottom: 6px;
            font-size: 13px;
            color: #d7e0e8;
        }

        .placeholder_text {
            font-size: 12px;
            color: #7f919f;
        }

        .inspector_panel_body {
            padding: 10px;
            overflow-x: hidden;
        }

        .inspector_summary {
            margin-bottom: 10px;
            padding: 10px 12px;
            background-color: #1e252c;
            border: 1px #2b3740;
        }

        .inspector_summary_title {
            margin-bottom: 4px;
            color: #d7e0e8;
            font-size: 13px;
        }

        .inspector_summary_text {
            color: #7f919f;
            font-size: 12px;
            white-space: normal;
        }

        .inspector_section,
        .inspector_foldout {
            margin-bottom: 10px;
            background-color: #1b2329;
            border: 1px #2c3943;
        }

        .inspector_field_row {
            display: flex;
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
            background-color: #1b2329;
            border: 1px #33414c;
            z-index: 40;
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

        #center_panel {
            position: absolute;
            overflow: hidden;
        }

        #viewport_panel {
            position: absolute;
            overflow: hidden;
            background-color: #10151a;
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
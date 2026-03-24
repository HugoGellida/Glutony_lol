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
            border-bottom-width: 1px;
            border-bottom-style: solid;
            border-bottom-color: #2a353e;
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
            border-width: 1px;
            border-style: solid;
            border-color: #2f3b45;
            z-index: 20;
        }

        .builder_menu_item {
            display: block;
            width: 100%;
            padding: 10px 14px;
            color: #d6dee5;
            border-bottom-width: 1px;
            border-bottom-style: solid;
            border-bottom-color: #2a353e;
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

        .panel_header {
            height: 34px;
            line-height: 34px;
            padding-left: 12px;
            background-color: #20272e;
            border-bottom-width: 1px;
            border-bottom-style: solid;
            border-bottom-color: #2f3b45;
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

        .placeholder_block {
            margin-bottom: 12px;
            padding: 10px 12px;
            background-color: #1e252c;
            border-width: 1px;
            border-style: solid;
            border-color: #2b3740;
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
            border-width: 1px;
            border-style: solid;
            border-color: #31404b;
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
            border-width: 1px;
            border-style: solid;
            border-color: #29353f;
        }

        .preview_canvas {
            width: 100%;
            flex: 1;
            position: relative;
            display: block;
            overflow: hidden;
            padding: 16px;
            background-color: #0d1318;
            border-width: 1px;
            border-style: solid;
            border-color: #28333c;
            box-sizing: border-box;
        }

        .preview_window {
            position: absolute;
            display: block;
            overflow: hidden;
            background-color: #ffffff;
            border-width: 2px;
            border-style: solid;
            border-color: #718391;
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
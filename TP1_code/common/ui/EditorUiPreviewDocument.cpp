#include "EditorUiPreviewDocument.hpp"

std::string createEmptyPreviewDocumentSource()
{
    return R"RML(<rml>
<head>
    <style>
        body {
            margin: 0px;
            width: 100%;
            height: 100%;
            background-color: #f8f4ea;
            color: #172028;
            font-family: LatoLatin;
        }
    </style>
</head>
<body>
</body>
</rml>
)RML";
}
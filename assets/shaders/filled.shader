{
    "Shaders":
    {
        "Vertex":{"Source":"assets/shaders/Color.vert"},
        "Fragment":{"Source":"assets/shaders/Color.frag"}
    },
    "DescriptorSetLayouts":
    [
        [
            {"Binding":0, "Count":1, "Type":"UniformBuffer"}
        ],
        [
            {"Binding":0, "Count":1, "Type":"CombinedImageSampler"},
            {"Binding":1, "Count":2, "Type":"CombinedImageSampler"}
        ],
        [
            {"Binding":0, "Count":1, "Type":"StorageBuffer"} 
        ],
        [
            {"Binding":0, "Count":4, "Type":"CombinedImageSampler"}
        ]
    ],
    "PushConstantRanges":
    [
        {"Size":4, "Offset":0}
    ],
    "ColorBlendState":
    {
        "LogicOp": "Copy",
        "logicOpEnable": false,
        "BlendConstants": [0,0,0,0],
        "ColorBlendAttachmentStates":
        [
            {
                "blendEnable": false,
                "SrcColorBlendFactor":"Zero",
                "DstColorBlendFactor":"Zero",
                "ColorBlendOp":"Add",
                "SrcAlphaBlendFactor":"Zero",
                "DstAlphaBlendFactor":"Zero",
                "AlphaBlendOp":"Add"
            }
        ]
    }
}
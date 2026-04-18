#version 330 core

in vec2 _uvs;

uniform sampler2D _sourceTex;
uniform sampler2D _sourceDepthTex;

out vec4 color;

void main()
{
    vec4 source = texture(_sourceTex, _uvs);
    vec3 hdrColor = max(source.rgb, vec3(0.0));
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    float coverage = clamp(source.a, 0.0, 1.0);
    if (coverage <= 0.0)
        discard;

    vec3 ldrColor = pow(mapped, vec3(1.0 / 2.2));
    gl_FragDepth = clamp(texture(_sourceDepthTex, _uvs).r, 0.0, 1.0);
    color = vec4(ldrColor * coverage, coverage);
}

#version 330 core

in vec2 _uvs;

uniform sampler2D _sourceTex;

out vec4 color;

void main()
{
    vec3 hdrColor = max(texture(_sourceTex, _uvs).rgb, vec3(0.0));
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    color = vec4(pow(mapped, vec3(1.0 / 2.2)), 1.0);
}

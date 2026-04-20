#version 330 core

in vec2 v_uvs;

uniform sampler2D _albedoMap;
uniform bool _albedoMap_present = false;
uniform float _opacity = 1.0;
uniform float _alphaCutoff = 0.5;

out float color;

void main()
{
        float alpha = _albedoMap_present ? texture(_albedoMap, v_uvs).a : 1.0;
        alpha = clamp(alpha * _opacity, 0.0, 1.0);
        if (alpha < clamp(_alphaCutoff, 0.0, 1.0))
                discard;

        color = 0.0;
}

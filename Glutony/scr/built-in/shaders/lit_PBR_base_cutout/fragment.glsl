#version 330 core

uniform vec3 _mainCol = vec3(1.0, 1.0, 1.0);
uniform sampler2D _albedoMap;
uniform bool _albedoMap_present = false;
uniform sampler2D _ambientOcclusionMap;
uniform bool _ambientOcclusionMap_present = false;
uniform float _ambientOcclusionValue = 1.0;
uniform float _opacity = 1.0;
uniform float _alphaCutoff = 0.5;

in vec2 v_uvs;

out vec4 color;

void main()
{
        vec4 albedoTexel = _albedoMap_present ? texture(_albedoMap, v_uvs) : vec4(1.0);
        float alpha = clamp(albedoTexel.a * _opacity, 0.0, 1.0);
        if (alpha < clamp(_alphaCutoff, 0.0, 1.0))
                discard;

        vec3 tint = clamp(_mainCol, vec3(0.0), vec3(1.0));
        vec3 albedo = pow(albedoTexel.rgb, vec3(2.2)) * tint;
        float ambientOcclusion = _ambientOcclusionMap_present ? texture(_ambientOcclusionMap, v_uvs).r : _ambientOcclusionValue;
        vec3 ambient = vec3(0.03) * albedo * clamp(ambientOcclusion, 0.0, 1.0);
        color = vec4(ambient, 1.0);
}

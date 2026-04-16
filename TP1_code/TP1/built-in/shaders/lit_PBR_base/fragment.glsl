#version 330 core

uniform sampler2D _albedoMap;
uniform sampler2D _ambientOcclusionMap;
uniform bool _ambientOcclusionMap_present = false;
uniform float _ambientOcclusionValue = 1.0;

in vec2 v_uvs;

out vec4 color;

vec3 sampleAlbedoLinear()
{
        return pow(texture(_albedoMap, v_uvs).rgb, vec3(2.2));
}

float sampleAmbientOcclusion()
{
        return _ambientOcclusionMap_present ? texture(_ambientOcclusionMap, v_uvs).r : _ambientOcclusionValue;
}

void main(){
        vec3 albedo = sampleAlbedoLinear();
        float ambientOcclusion = clamp(sampleAmbientOcclusion(), 0.0, 1.0);
        vec3 ambient = vec3(0.03) * albedo * ambientOcclusion;
        color = vec4(ambient, 1.0);
}

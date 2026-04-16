#version 330 core

in vec3 _normals;
in vec4 _clipPosition;
in vec2 _uvs;
in vec3 _colors;
in vec3 _worldPos;
uniform sampler2D _albedoMap;
uniform sampler2D _roughnessMap;


uniform int _lightType = 0;
uniform vec3 _lightPos = vec3(0.0f, 0.0f, 0.0f);
uniform vec3 _lightDir = vec3(1.0f, 0.75f, -0.5f);
uniform vec3 _lightColor = vec3(1.0f, 1.0f, 1.0f);
uniform float _lightIntensity = 1.0f;
uniform sampler2D _lightOcclusionTex;

uniform vec3 _CAMPOS;
// Ouput data
out vec3 color;


const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = max(dot(N, H), 0.0);
        float NdotH2 = NdotH * NdotH;

        float num = a2;
        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
        denom = PI * denom * denom;

        return num / denom;
}




void main(){
        vec3 normal = normalize(_normals);
        vec3 V = normalize(_CAMPOS - _worldPos);

        vec3 L = _lightType == 0 ? normalize(-_lightDir) : normalize(_lightPos - _worldPos);
        float attenuation = 1.0f;
        if (_lightType == 1)
        {
                float lightDistance = length(_lightPos - _worldPos);
                attenuation = 1.0f / max(1.0f, lightDistance * lightDistance * 0.1f);
        }

        vec3 H = normalize(V + L);

        vec3 radiance = _lightColor * attenuation;

        // cook-torrance BRDF
        float NDF = DistributionGGX(normal, H, texture(_roughnessMap, _uvs).r);



        float ndl = max(dot(normal, L), 0.0f);
        vec2 occlusionUv = (_clipPosition.xy / max(_clipPosition.w, 1e-6f)) * 0.5f + 0.5f;
        occlusionUv = clamp(occlusionUv, vec2(0.0f), vec2(1.0f));
        float occlusion = clamp(texture(_lightOcclusionTex, occlusionUv).r, 0.0f, 1.0f);
        color = texture(_albedoMap, _uvs).rgb * _lightColor * (_lightIntensity * ndl * attenuation * occlusion);
}

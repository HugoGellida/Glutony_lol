#version 330 core

in vec3 _normals;
in vec4 _clipPosition;
in vec2 _uvs;
in vec3 _colors;
in vec3 _worldPos;
uniform sampler2D _albedoMap;
uniform sampler2D _metallicMap;
uniform sampler2D _roughnessMap;
uniform bool _metallicMap_present = false;
uniform bool _roughnessMap_present = false;
uniform float _metallicValue = 0.0;
uniform float _roughnessValue = 0.5;

uniform int _lightType = 0;
uniform vec3 _lightPos = vec3(0.0f, 0.0f, 0.0f);
uniform vec3 _lightDir = vec3(1.0f, 0.75f, -0.5f);
uniform vec3 _lightColor = vec3(1.0f, 1.0f, 1.0f);
uniform float _lightIntensity = 1.0f;
uniform sampler2D _lightOcclusionTex;

uniform vec3 _CAMPOS;
// Output data
out vec3 color;

const float PI = 3.14159265359;

vec3 sampleAlbedoLinear()
{
        return pow(texture(_albedoMap, _uvs).rgb, vec3(2.2));
}

float sampleMetallic()
{
        return _metallicMap_present ? texture(_metallicMap, _uvs).r : _metallicValue;
}

float sampleRoughness()
{
        return _roughnessMap_present ? texture(_roughnessMap, _uvs).r : _roughnessValue;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = max(dot(N, H), 0.0);
        float NdotH2 = NdotH * NdotH;

        float num = a2;
        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
        denom = PI * denom * denom;

        return num / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
        float r = roughness + 1.0;
        float k = (r * r) / 8.0;

        float num = NdotV;
        float denom = NdotV * (1.0 - k) + k;

        return num / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        float ggxV = GeometrySchlickGGX(NdotV, roughness);
        float ggxL = GeometrySchlickGGX(NdotL, roughness);
        return ggxV * ggxL;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
        return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main(){
        vec3 normal = normalize(_normals);
        vec3 albedo = sampleAlbedoLinear();
        float metallic = clamp(sampleMetallic(), 0.0, 1.0);
        float roughness = clamp(sampleRoughness(), 0.04, 1.0);
        vec3 V = normalize(_CAMPOS - _worldPos);

        vec3 L = _lightType == 0 ? normalize(-_lightDir) : normalize(_lightPos - _worldPos);
        float attenuation = 1.0;
        if (_lightType == 1)
        {
                float lightDistance = length(_lightPos - _worldPos);
                attenuation = 1.0 / max(lightDistance * lightDistance, 0.0001);
        }

        vec3 H = normalize(V + L);
        float NdotL = max(dot(normal, L), 0.0);
        float NdotV = max(dot(normal, V), 0.0);

        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, V, L, roughness);

        vec3 numerator = NDF * G * F;
        float denominator = max(4.0 * NdotV * NdotL, 0.0001);
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 radiance = _lightColor * (_lightIntensity * attenuation);

        vec2 occlusionUv = (_clipPosition.xy / max(_clipPosition.w, 0.000001)) * 0.5 + 0.5;
        occlusionUv = clamp(occlusionUv, vec2(0.0), vec2(1.0));
        float occlusion = clamp(texture(_lightOcclusionTex, occlusionUv).r, 0.0, 1.0);

        color = (kD * albedo / PI + specular) * radiance * NdotL * occlusion;
}

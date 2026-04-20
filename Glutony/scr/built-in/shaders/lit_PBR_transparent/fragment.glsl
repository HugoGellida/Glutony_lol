#version 330 core

in vec3 _normals;
in vec2 _uvs;
in vec3 _worldPos;
in mat3 _TBN;

uniform vec3 _mainCol = vec3(1.0, 1.0, 1.0);
uniform sampler2D _albedoMap;
uniform sampler2D _metallicMap;
uniform sampler2D _roughnessMap;
uniform sampler2D _normalMap;
uniform sampler2D _ambientOcclusionMap;
uniform bool _albedoMap_present = false;
uniform bool _metallicMap_present = false;
uniform bool _roughnessMap_present = false;
uniform bool _normalMap_present = false;
uniform bool _ambientOcclusionMap_present = false;
uniform float _metallicValue = 0.0;
uniform float _roughnessValue = 0.5;
uniform float _ambientOcclusionValue = 1.0;
uniform float _opacity = 1.0;

uniform int _lightCount = 0;
uniform int _lightType0 = 0;
uniform int _lightType1 = 0;
uniform int _lightType2 = 0;
uniform int _lightType3 = 0;
uniform int _lightType4 = 0;
uniform int _lightType5 = 0;
uniform int _lightType6 = 0;
uniform int _lightType7 = 0;
uniform vec3 _lightPos0 = vec3(0.0);
uniform vec3 _lightPos1 = vec3(0.0);
uniform vec3 _lightPos2 = vec3(0.0);
uniform vec3 _lightPos3 = vec3(0.0);
uniform vec3 _lightPos4 = vec3(0.0);
uniform vec3 _lightPos5 = vec3(0.0);
uniform vec3 _lightPos6 = vec3(0.0);
uniform vec3 _lightPos7 = vec3(0.0);
uniform vec3 _lightDir0 = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightDir1 = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightDir2 = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightDir3 = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightDir4 = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightDir5 = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightDir6 = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightDir7 = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightColor0 = vec3(1.0);
uniform vec3 _lightColor1 = vec3(1.0);
uniform vec3 _lightColor2 = vec3(1.0);
uniform vec3 _lightColor3 = vec3(1.0);
uniform vec3 _lightColor4 = vec3(1.0);
uniform vec3 _lightColor5 = vec3(1.0);
uniform vec3 _lightColor6 = vec3(1.0);
uniform vec3 _lightColor7 = vec3(1.0);
uniform float _lightIntensity0 = 0.0;
uniform float _lightIntensity1 = 0.0;
uniform float _lightIntensity2 = 0.0;
uniform float _lightIntensity3 = 0.0;
uniform float _lightIntensity4 = 0.0;
uniform float _lightIntensity5 = 0.0;
uniform float _lightIntensity6 = 0.0;
uniform float _lightIntensity7 = 0.0;
uniform vec3 _CAMPOS = vec3(0.0);

out vec4 color;

const int MAX_LIGHTS = 8;
const float PI = 3.14159265359;

vec3 normalizeOr(vec3 value, vec3 fallback)
{
        float valueLength = length(value);
        return valueLength > 0.0001 ? value / valueLength : fallback;
}

int getLightType(int lightIndex)
{
        switch (lightIndex)
        {
        case 0: return _lightType0;
        case 1: return _lightType1;
        case 2: return _lightType2;
        case 3: return _lightType3;
        case 4: return _lightType4;
        case 5: return _lightType5;
        case 6: return _lightType6;
        case 7: return _lightType7;
        default: return 0;
        }
}

vec3 getLightPos(int lightIndex)
{
        switch (lightIndex)
        {
        case 0: return _lightPos0;
        case 1: return _lightPos1;
        case 2: return _lightPos2;
        case 3: return _lightPos3;
        case 4: return _lightPos4;
        case 5: return _lightPos5;
        case 6: return _lightPos6;
        case 7: return _lightPos7;
        default: return vec3(0.0);
        }
}

vec3 getLightDir(int lightIndex)
{
        switch (lightIndex)
        {
        case 0: return _lightDir0;
        case 1: return _lightDir1;
        case 2: return _lightDir2;
        case 3: return _lightDir3;
        case 4: return _lightDir4;
        case 5: return _lightDir5;
        case 6: return _lightDir6;
        case 7: return _lightDir7;
        default: return vec3(1.0, 0.75, -0.5);
        }
}

vec3 getLightColor(int lightIndex)
{
        switch (lightIndex)
        {
        case 0: return _lightColor0;
        case 1: return _lightColor1;
        case 2: return _lightColor2;
        case 3: return _lightColor3;
        case 4: return _lightColor4;
        case 5: return _lightColor5;
        case 6: return _lightColor6;
        case 7: return _lightColor7;
        default: return vec3(0.0);
        }
}

float getLightIntensity(int lightIndex)
{
        switch (lightIndex)
        {
        case 0: return _lightIntensity0;
        case 1: return _lightIntensity1;
        case 2: return _lightIntensity2;
        case 3: return _lightIntensity3;
        case 4: return _lightIntensity4;
        case 5: return _lightIntensity5;
        case 6: return _lightIntensity6;
        case 7: return _lightIntensity7;
        default: return 0.0;
        }
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = max(dot(N, H), 0.0);
        float NdotH2 = NdotH * NdotH;

        float num = a2;
        float denom = NdotH2 * (a2 - 1.0) + 1.0;
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

vec3 sampleSurfaceNormal()
{
        vec3 geometricNormal = normalize(_normals);
        if (!_normalMap_present)
                return geometricNormal;

        vec3 tangentNormal = texture(_normalMap, _uvs).xyz * 2.0 - 1.0;
        mat3 tbn = mat3(normalize(_TBN[0]), normalize(_TBN[1]), normalize(_TBN[2]));
        return normalize(tbn * normalize(tangentNormal));
}

void main()
{
        vec4 albedoTexel = _albedoMap_present ? texture(_albedoMap, _uvs) : vec4(1.0);
        float alpha = clamp(albedoTexel.a * _opacity, 0.0, 1.0);
        if (alpha <= 0.0)
                discard;

        vec3 tint = clamp(_mainCol, vec3(0.0), vec3(1.0));
        vec3 albedo = pow(albedoTexel.rgb, vec3(2.2)) * tint;
        float metallic = clamp(_metallicMap_present ? texture(_metallicMap, _uvs).r : _metallicValue, 0.0, 1.0);
        float roughness = clamp(_roughnessMap_present ? texture(_roughnessMap, _uvs).r : _roughnessValue, 0.04, 1.0);
        float ambientOcclusion = clamp(_ambientOcclusionMap_present ? texture(_ambientOcclusionMap, _uvs).r : _ambientOcclusionValue, 0.0, 1.0);
        vec3 normal = sampleSurfaceNormal();
        vec3 viewVector = normalizeOr(_CAMPOS - _worldPos, vec3(0.0, 0.0, 1.0));

        vec3 totalColor = vec3(0.03) * albedo * ambientOcclusion;

        for (int lightIndex = 0; lightIndex < MAX_LIGHTS; ++lightIndex)
        {
                if (lightIndex >= _lightCount)
                        break;

                int lightType = getLightType(lightIndex);
                vec3 lightPos = getLightPos(lightIndex);
                vec3 lightDir = normalizeOr(getLightDir(lightIndex), vec3(1.0, 0.75, -0.5));
                vec3 lightColor = getLightColor(lightIndex);
                float lightIntensity = getLightIntensity(lightIndex);

                vec3 lightVector = lightType == 0
                        ? normalizeOr(-lightDir, vec3(0.0, 0.0, -1.0))
                        : normalizeOr(lightPos - _worldPos, vec3(0.0, 0.0, -1.0));
                float attenuation = 1.0;
                if (lightType == 1)
                {
                        float lightDistance = length(lightPos - _worldPos);
                        attenuation = 1.0 / max(1.0, lightDistance * lightDistance * 0.1);
                }

                vec3 halfVector = normalizeOr(viewVector + lightVector, lightVector);
                float NdotL = max(dot(normal, lightVector), 0.0);
                float NdotV = max(dot(normal, viewVector), 0.0);
                if (NdotL <= 0.0 || NdotV <= 0.0 || lightIntensity <= 0.0)
                        continue;

                vec3 F0 = mix(vec3(0.04), albedo, metallic);
                vec3 F = fresnelSchlick(max(dot(halfVector, viewVector), 0.0), F0);
                float NDF = DistributionGGX(normal, halfVector, roughness);
                float G = GeometrySmith(normal, viewVector, lightVector, roughness);

                vec3 numerator = NDF * G * F;
                float denominator = max(4.0 * NdotV * NdotL, 0.0001);
                vec3 specular = numerator / denominator;

                vec3 kS = F;
                vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
                vec3 radiance = lightColor * (lightIntensity * attenuation);
                totalColor += (kD * albedo / PI + specular) * radiance * NdotL;
        }

        vec3 hdrColor = max(totalColor, vec3(0.0));
        vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
        vec3 ldrColor = pow(mapped, vec3(1.0 / 2.2));
        color = vec4(ldrColor * alpha, alpha);
}

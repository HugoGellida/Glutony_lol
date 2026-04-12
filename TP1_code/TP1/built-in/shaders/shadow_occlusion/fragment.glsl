#version 330 core

in vec3 _normals;
in vec3 _worldPos;

uniform sampler2D _shadowDepthTex;
uniform mat4 LIGHT_MVP;
uniform int _lightType = 0;
uniform vec3 _lightPos = vec3(0.0f, 0.0f, 0.0f);
uniform vec3 _lightDir = vec3(1.0f, 0.75f, -0.5f);
uniform int _shadowFaceIndex = 0;
uniform float _shadowBiasBase = 0.0005f;
uniform float _shadowBiasSlopeScale = 0.005f;
uniform float _shadowBiasClamp = 0.02f;
uniform int _shadowPcfRadius = 1;
uniform float _shadowStrength = 1.0f;

out float color;

int dominantPointLightFace(vec3 direction)
{
        vec3 absDirection = abs(direction);
        if (absDirection.x >= absDirection.y && absDirection.x >= absDirection.z)
                return direction.x >= 0.0f ? 0 : 1;
        if (absDirection.y >= absDirection.x && absDirection.y >= absDirection.z)
                return direction.y >= 0.0f ? 2 : 3;
        return direction.z >= 0.0f ? 4 : 5;
}

float sampleShadowVisibility(vec2 shadowUv, float currentDepth, float bias)
{
        int pcfRadius = clamp(_shadowPcfRadius, 0, 4);
        vec2 texelSize = 1.0f / vec2(textureSize(_shadowDepthTex, 0));
        float litSamples = 0.0f;
        float sampleCount = 0.0f;

        for (int offsetY = -4; offsetY <= 4; ++offsetY)
        {
                if (abs(offsetY) > pcfRadius)
                        continue;

                for (int offsetX = -4; offsetX <= 4; ++offsetX)
                {
                        if (abs(offsetX) > pcfRadius)
                                continue;

                        float storedDepth = texture(_shadowDepthTex, shadowUv + vec2(float(offsetX), float(offsetY)) * texelSize).r;
                        litSamples += currentDepth - bias <= storedDepth ? 1.0f : 0.0f;
                        sampleCount += 1.0f;
                }
        }

        return sampleCount > 0.0f ? litSamples / sampleCount : 1.0f;
}

void main(){
        if (_lightType == 1 && dominantPointLightFace(_worldPos - _lightPos) != _shadowFaceIndex)
        {
                color = 0.0f;
                return;
        }

        vec4 lightClip = LIGHT_MVP * vec4(_worldPos, 1.0f);
        if (lightClip.w <= 0.0f)
        {
                color = 0.0f;
                return;
        }

        vec3 projected = lightClip.xyz / lightClip.w;
        vec3 shadowUv = projected * 0.5f + 0.5f;
        if (shadowUv.x < 0.0f || shadowUv.x > 1.0f || shadowUv.y < 0.0f || shadowUv.y > 1.0f || shadowUv.z < 0.0f || shadowUv.z > 1.0f)
        {
                color = 1.0f;
                return;
        }

        vec3 normal = normalize(_normals);
        vec3 lightVector = _lightType == 0 ? normalize(-_lightDir) : normalize(_lightPos - _worldPos);
        float ndl = abs(dot(normal, lightVector));
        float bias = clamp(_shadowBiasBase + _shadowBiasSlopeScale * (1.0f - ndl), _shadowBiasBase, _shadowBiasClamp);
        float currentDepth = shadowUv.z;
        float visibility = sampleShadowVisibility(shadowUv.xy, currentDepth, bias);
        float shadowStrength = clamp(_shadowStrength, 0.0f, 1.0f);
        color = 1.0f - (1.0f - visibility) * shadowStrength;
}
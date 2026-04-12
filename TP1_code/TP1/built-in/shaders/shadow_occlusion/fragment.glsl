#version 330 core

in vec3 _normals;
in vec3 _worldPos;

uniform sampler2D _shadowDepthTex;
uniform mat4 LIGHT_MVP;
uniform int _lightType = 0;
uniform vec3 _lightPos = vec3(0.0f, 0.0f, 0.0f);
uniform vec3 _lightDir = vec3(1.0f, 0.75f, -0.5f);
uniform int _shadowFaceIndex = 0;

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
        float bias = max(0.0005f, 0.005f * (1.0f - max(dot(normal, lightVector), 0.0f)));
        float storedDepth = texture(_shadowDepthTex, shadowUv.xy).r;
        float currentDepth = shadowUv.z;
        color = currentDepth - bias <= storedDepth ? 1.0f : 0.0f;
}
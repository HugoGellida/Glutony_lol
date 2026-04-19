#version 330 core

in vec3 _normals;
in vec4 _clipPosition;
in vec3 _worldPos;

uniform vec3 _mainCol = vec3(1.0, 1.0, 1.0);
uniform float _metallicValue = 0.0;
uniform float _roughnessValue = 0.5;

uniform int _lightType = 0;
uniform vec3 _lightPos = vec3(0.0, 0.0, 0.0);
uniform vec3 _lightDir = vec3(1.0, 0.75, -0.5);
uniform vec3 _lightColor = vec3(1.0, 1.0, 1.0);
uniform float _lightIntensity = 1.0;
uniform sampler2D _lightOcclusionTex;

uniform vec3 _CAMPOS = vec3(0.0, 0.0, 0.0);

out vec3 color;

void main()
{
        vec3 albedo = clamp(_mainCol, vec3(0.0), vec3(1.0));
        float metallic = clamp(_metallicValue, 0.0, 1.0);
        float roughness = clamp(_roughnessValue, 0.04, 1.0);

        vec3 normal = normalize(_normals);
        vec3 viewVector = normalize(_CAMPOS - _worldPos);
        vec3 lightVector = _lightType == 0 ? normalize(-_lightDir) : normalize(_lightPos - _worldPos);

        float attenuation = 1.0;
        if (_lightType == 1)
        {
                float lightDistance = length(_lightPos - _worldPos);
                attenuation = 1.0 / max(1.0, lightDistance * lightDistance * 0.1);
        }

        float ndl = max(dot(normal, lightVector), 0.0);
        vec3 halfVector = normalize(viewVector + lightVector);
        float shininess = mix(128.0, 4.0, roughness);
        float specularTerm = pow(max(dot(normal, halfVector), 0.0), shininess);

        vec3 diffuseColor = albedo * (1.0 - metallic);
        vec3 specularColor = mix(vec3(0.04), albedo, metallic);
        vec3 radiance = _lightColor * (_lightIntensity * attenuation);

        vec2 occlusionUv = (_clipPosition.xy / max(_clipPosition.w, 1e-6)) * 0.5 + 0.5;
        occlusionUv = clamp(occlusionUv, vec2(0.0), vec2(1.0));
        float occlusion = clamp(texture(_lightOcclusionTex, occlusionUv).r, 0.0, 1.0);

        vec3 diffuse = diffuseColor * ndl;
        vec3 specular = specularColor * specularTerm * step(0.0, ndl);
        color = (diffuse + specular) * radiance * occlusion;
}

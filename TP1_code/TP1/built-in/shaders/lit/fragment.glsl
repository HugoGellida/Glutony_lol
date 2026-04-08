#version 330 core

in vec3 _normals;
in vec2 _uvs;
in vec3 _colors;
in vec3 _worldPos;
uniform vec3 _mainCol = vec3(1.0f, 1.0f, 1.0f);
uniform int _lightType = 0;
uniform vec3 _lightPos = vec3(0.0f, 0.0f, 0.0f);
uniform vec3 _lightDir = vec3(1.0f, 0.75f, -0.5f);
uniform vec3 _lightColor = vec3(1.0f, 1.0f, 1.0f);
uniform float _lightIntensity = 1.0f;
// Ouput data
out vec3 color;


void main(){
        vec3 normal = normalize(_normals);
        vec3 lightVector = _lightType == 0 ? normalize(-_lightDir) : normalize(_lightPos - _worldPos);
        float attenuation = 1.0f;
        if (_lightType == 1)
        {
                float lightDistance = length(_lightPos - _worldPos);
                attenuation = 1.0f / max(1.0f, lightDistance * lightDistance * 0.1f);
        }

        float ndl = max(dot(normal, lightVector), 0.0f);
        color = _mainCol * _lightColor * (_lightIntensity * ndl * attenuation);
}

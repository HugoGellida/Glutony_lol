#version 330 core

in vec2 _uvs;

uniform sampler2D _sourceTex;
uniform vec3 _tint = vec3(1.0, 1.0, 1.0);

out vec4 color;

void main(){
        vec3 sampled = texture(_sourceTex, _uvs).rgb;
        color = vec4(sampled * _tint, 1.0);
}
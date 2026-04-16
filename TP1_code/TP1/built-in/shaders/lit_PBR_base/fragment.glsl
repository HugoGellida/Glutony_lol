#version 330 core

uniform sampler2D _albedoMap;

in vec2 v_uvs;

out vec4 color;

void main(){
        color = vec4(texture(_albedoMap, v_uvs).rgb, 1.0);
}
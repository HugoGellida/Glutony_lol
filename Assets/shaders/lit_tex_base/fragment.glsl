#version 330 core

uniform vec3 _mainCol = vec3(1.0, 1.0, 1.0);


in vec2 v_uvs;

out vec4 color;

void main(){
        color = vec4(_mainCol * 0.4, 1.0);
}
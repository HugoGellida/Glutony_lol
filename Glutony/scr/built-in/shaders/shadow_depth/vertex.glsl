#version 330 core

layout(location = 0) in vec3 vertices;

uniform mat4 LIGHT_MVP;

void main(){
        gl_Position = LIGHT_MVP * vec4(vertices, 1.0);
}
#version 330 core

layout(location = 0) in vec3 vertices;

layout(location = 2) in vec2 uvs;

uniform mat4 MVP;

out vec2 v_uvs;
void main(){
        gl_Position = MVP * vec4(vertices, 1.0);
        v_uvs = uvs;
}
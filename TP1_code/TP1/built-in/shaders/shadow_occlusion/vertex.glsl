#version 330 core

layout(location = 0) in vec3 vertices;
layout(location = 1) in vec3 normals;

uniform mat4 MVP;
uniform mat4 MVP_ORTHO;
uniform mat4 MODEL;

out vec3 _normals;
out vec3 _worldPos;

void main(){
        gl_Position = MVP * vec4(vertices, 1.0);
        _normals = (MVP_ORTHO * vec4(normals, 0.0)).xyz;
        _worldPos = (MODEL * vec4(vertices, 1.0)).xyz;
}
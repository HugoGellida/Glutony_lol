#version 330 core

layout(location = 0) in vec3 vertices;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec2 uvs;

out vec2 _uvs;

void main(){
        gl_Position = vec4(vertices.x * 0.2, vertices.z * 0.2, 0.0, 1.0);
        _uvs = uvs;
}
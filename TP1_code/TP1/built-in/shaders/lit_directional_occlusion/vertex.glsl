#version 330 core

layout(location = 0) in vec3 vertices;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec2 uvs;
layout(location = 3) in vec3 colors;

uniform mat4 MVP;
uniform mat4 MVP_ORTHO;
uniform mat4 MODEL;

out vec3 _normals;
out vec4 _clipPosition;
out vec2 _uvs;
out vec3 _colors;
out vec3 _worldPos;

void main(){
        gl_Position = MVP * vec4(vertices, 1);
        _clipPosition = gl_Position;
        _normals = (MVP_ORTHO * vec4(normals, 0)).xyz;
        _uvs = uvs;
        _colors = colors;
        _worldPos = (MODEL * vec4(vertices, 1)).xyz;
}
#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec2 uvs;
layout(location = 3) in vec3 colors;

//TODO create uniform transformations matrices Model View Projection
// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform mat4 MVP_ORTHO; 

out vec3 _normals;
out vec2 _uvs;
out vec3 _colors;

void main(){
        gl_Position = MVP * vec4(vertices, 1);
        _normals = (MVP_ORTHO * vec4(normals, 0)).xyz;
        _uvs = uvs;
        _colors = colors;
}


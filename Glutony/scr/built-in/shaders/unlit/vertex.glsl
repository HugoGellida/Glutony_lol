#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices;
layout(location = 1) in vec3 normals;

uniform mat4 MVP;
uniform mat4 MVP_ORTHO; 

out vec3 _normals;
void main(){
        gl_Position = MVP * vec4(vertices, 1);
}


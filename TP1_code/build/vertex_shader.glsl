#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 2) in vec2 uvs;

//TODO create uniform transformations matrices Model View Projection
// Values that stay constant for the whole mesh.
uniform mat4 MVP;

out vec2 texture_Coordinates;

void main(){
        mat4 dbg = mat4(1.0, 0.0, 0.0, 0.0,
                        0.0, 1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 1.0);
        // TODO : Output position of the vertex, in clip space : MVP * position
        gl_Position = MVP * vec4(vertices_position_modelspace,1);
        texture_Coordinates = uvs;
}


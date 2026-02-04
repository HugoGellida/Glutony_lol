#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 2) in vec2 uvs;

//TODO create uniform transformations matrices Model View Projection
// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform sampler2D height_map;

out float height;
out vec2 texture_Coordinates;

void main(){
        mat4 dbg = mat4(1.0, 0.0, 0.0, 0.0,
                        0.0, 1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 1.0);
        float vert = ((texture(height_map, uvs).r * 2.0) - 1.0) + vertices_position_modelspace.y;
        // TODO : Output position of the vertex, in clip space : MVP * position
        gl_Position = MVP * vec4(vertices_position_modelspace.x, vert, vertices_position_modelspace.z,1);
        texture_Coordinates = uvs;
        height = vert;
}


#version 330 core
in vec2 texture_Coordinates;

uniform sampler2D main_tex;

// Ouput data
out vec3 color;

void main(){

        color =texture(main_tex, texture_Coordinates).rgb;

}

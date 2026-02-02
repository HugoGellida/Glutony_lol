#version 330 core
in vec2 texture_Coordinates;
// Ouput data
out vec3 color;

void main(){

        color =vec3(texture_Coordinates.xy,0.4);

}

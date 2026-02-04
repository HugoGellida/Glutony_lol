#version 330 core
in vec2 texture_Coordinates;
in float height;
uniform sampler2D t0, t1, t2;
// Ouput data
out vec3 color;

vec3 lerp(float t, vec3 a, vec3 b)
{
        return (1.0 - t) * a + t * b;
}


float remap(float min, float max, float t)
{
        return clamp(0, 1,(t - min) / (max - min));
}


void main(){
        vec3 c0 = texture(t0, texture_Coordinates).rgb;
        vec3 c1 = texture(t1, texture_Coordinates).rgb;
        vec3 c2 = texture(t2, texture_Coordinates).rgb;
        vec3 l0 = lerp(remap(-0.6, -0.2, height), c0, c1);
        vec3 l1 = lerp(remap(0.2, 0.6, height), c1, c2);
        bool P = height > -0.6 && height < -0.2 || height > 0.2 && height < 0.6;
        color = !P ? (height >= 0.6 ? c2 : height <= -0.6 ? c0 : c1) 
                : (height > 0.2 && height < 0.6 ? l1 : l0);

}

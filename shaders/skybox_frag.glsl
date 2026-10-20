#version 410


uniform samplerCube skyboxMap;

out vec4 fragColor;

in vec3 fragPosition;

void main()
{
    vec3 color = texture(skyboxMap, fragPosition).xyz;

    color = color / (color + vec3(1.0)); // Same gamma correction as the PBR shader
    color = pow(color, vec3(1.0/2.2));

    fragColor = vec4(color, 1.0); 

}

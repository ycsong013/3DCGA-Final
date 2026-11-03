#version 410


uniform samplerCube skyboxMapMorning;
uniform samplerCube skyboxMapNight;
uniform float interpolationFactor;

out vec4 fragColor;

in vec3 fragPosition;

void main()
{
    vec3 color1 = texture(skyboxMapMorning, fragPosition).xyz;
    vec3 color2 = texture(skyboxMapNight, fragPosition).xyz;
    vec3 finalColor = mix(color1, color2, interpolationFactor);

    finalColor = finalColor / (finalColor + vec3(1.0)); // Same gamma correction as the PBR shader
    finalColor = pow(finalColor, vec3(1.0/2.2));

    fragColor = vec4(finalColor, 1.0); 

}

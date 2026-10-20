#version 410

// For the HDR map - cube conversion, we have used the LearnOpenGL tutorial. 
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// Again,
// what we did was to take a look at the tutorial and learn from it, and write the 
// main points down. Later, we tried to implement the code without looking at the
// tutorial code available on the website.

uniform sampler2D environmentMap;

out vec4 fragColor;

in vec3 fragPosition;

void main()
{
    vec3 positionNormalized = normalize(fragPosition); // Each position in the environment map is a sample direction, so we think of it as a direction.
    vec2 fragTexCoord = vec2(atan(positionNormalized.z, positionNormalized.x), asin(positionNormalized.y)); // Converting the equirectangular coordinates to the cartesian space (2D).

    fragTexCoord = fragTexCoord * vec2(0.1591, 0.3183) + 0.5; // To scale the coordinates to a 0 to 1 range
    fragColor = vec4(texture(environmentMap, fragTexCoord).xyz, 1.0); 

}

#version 410

// Convolution shader used to pre-compute the diffuse component of the environment 
// influenced irradiance. The concepts to build this shader were learned from 
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance. After learning the concept and noting
// them down, we wrote the code without looking at the code available on the website.

uniform samplerCube skyboxMap;

out vec4 fragColor;

in vec3 fragPosition; // Treat each position as the normal of a hemisphere, and turn the coordinates
// so that they are all centered around the "world" of the fragment's position

void main()
{
    vec3 irradiance = vec3(0.0f);
    // Build a new coordinate hemisphere plane around the position of the fragment.
    vec3 zDirection = normalize(fragPosition);
    vec3 yDirection = vec3(0.0f, 1.0f, 0.0f); // Just a helper vector, and not exactly perpendicular to z
    vec3 xDirection = normalize(cross(yDirection, zDirection)); // This way, x is certainly perpendicular to z.
    yDirection = normalize(cross(zDirection, xDirection));

    float samplingStep = 0.025;

    int numOfSamples = 0;

    for (float phi = 0.0; phi < 2.0 * radians(180); phi += samplingStep) {
        for (float theta = 0.0; theta < radians(180)/2.0; theta += samplingStep) {
            vec3 cartesianVector = vec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
            cartesianVector = cartesianVector.x * xDirection + cartesianVector.y * yDirection + cartesianVector.z * zDirection;

            irradiance += texture(skyboxMap, cartesianVector).xyz * cos(theta) * sin(theta);
            numOfSamples++;

        }
    }

    irradiance = irradiance * radians(180) / numOfSamples;

    fragColor = vec4(irradiance, 1.0);

}

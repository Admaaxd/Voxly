#version 430 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float visibility;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 fogColor;

void main()
{
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 litColor = (ambient + diffuse) * objectColor;

    vec3 finalColor = mix(fogColor, litColor, visibility);
    FragColor = vec4(finalColor, 1.0);
}

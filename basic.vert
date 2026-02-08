#version 410 core
layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 uvTiling;
uniform vec2 uvMin;
uniform vec2 uvMax;
uniform vec2 uvOffset;   // ADD THIS

void main()
{
    fPosition = vPosition;
    fNormal   = vNormal;

    vec2 cropped = mix(uvMin, uvMax, vTexCoords);
    fTexCoords = cropped * uvTiling + uvOffset;  // ADD OFFSET

    gl_Position = projection * view * model * vec4(vPosition, 1.0);
}

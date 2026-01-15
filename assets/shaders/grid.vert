#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProj;

out vec4 vColor;
out vec3 vWorldPos;

void main() {
    vColor = aColor;
    vWorldPos = aPos; // grid in world space
    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
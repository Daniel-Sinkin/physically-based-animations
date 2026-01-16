#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uColor;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(0.35, 0.55, 0.75));
    float diff = max(dot(N, L), 0.0);

    vec3 base = uColor * (0.20 + 0.80 * diff);

    FragColor = vec4(base, 1.0);
}
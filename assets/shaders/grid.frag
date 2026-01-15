#version 330 core
in vec4 vColor;
in vec3 vWorldPos;

uniform float uFogStart;
uniform float uFogEnd;

out vec4 FragColor;

void main() {
    float d = length(vWorldPos.xy);
    float t = clamp((d - uFogStart) / max(1e-6, (uFogEnd - uFogStart)), 0.0, 1.0);
    float fog = 1.0 - t;

    float alpha = vColor.a * fog;
    FragColor = vec4(vColor.rgb, alpha);
}
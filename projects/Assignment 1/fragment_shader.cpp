#version 330 core
in vec2 uv;
out vec4 FragColor;

uniform int pixelArt[64];   // 8x8
uniform vec3 palette[3];    // ÈýÖÖÑÕÉ«

void main() {
    int x = int(floor(uv.x * 8.0));
    int y = int(floor((1.0 - uv.y) * 8.0));
    int idx = y * 8 + x;

    int colorIndex = pixelArt[idx];
    vec3 color = palette[colorIndex];

    FragColor = vec4(color, 1.0);
}

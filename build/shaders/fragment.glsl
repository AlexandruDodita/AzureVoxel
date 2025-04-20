#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 blockColor;
uniform sampler2D blockTexture;
uniform bool useTexture;

void main() {
    if (useTexture) {
        FragColor = texture(blockTexture, TexCoord);
    } else {
        // Minecraft-like grass block color
        vec3 grassColor = vec3(0.333, 0.549, 0.333);
        FragColor = vec4(grassColor, 1.0);
    }
}

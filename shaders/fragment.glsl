#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 blockColor;
uniform sampler2D blockTexture;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 texColor = texture(blockTexture, TexCoord);
        // Make sure texture with alpha is handled correctly
        if(texColor.a < 0.1)
            discard;
        FragColor = texColor;
    } else {
        // When no texture is available, use a solid color
        // Different colors for debugging
        vec3 color;
        if (gl_FrontFacing) {
            color = blockColor; // Use the provided block color
        } else {
            color = vec3(1.0, 0.0, 0.0); // Red for back faces (should not be visible normally)
        }
        FragColor = vec4(color, 1.0);
    }
}

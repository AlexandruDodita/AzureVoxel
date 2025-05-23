#include "../headers/crosshair.h"
#include <vector>

Crosshair::Crosshair(int screenWidth, int screenHeight)
    : shader_("shaders/crosshair_vertex.glsl", "shaders/crosshair_fragment.glsl"),
      screenWidth_(screenWidth),
      screenHeight_(screenHeight) {
    projection_ = glm::ortho(0.0f, static_cast<float>(screenWidth_),
                             0.0f, static_cast<float>(screenHeight_));
    setupMesh();
}

Crosshair::~Crosshair() {
    glDeleteVertexArrays(1, &VAO_);
    glDeleteBuffers(1, &VBO_);
}

void Crosshair::setupMesh() {
    float crosshairSize = 20.0f; // Length of each line of the crosshair
    float thickness = 2.0f; // Thickness of the lines

    // Center of the screen
    float centerX = screenWidth_ / 2.0f;
    float centerY = screenHeight_ / 2.0f;

    std::vector<glm::vec2> vertices = {
        // Horizontal line
        {centerX - crosshairSize / 2, centerY - thickness / 2},
        {centerX + crosshairSize / 2, centerY - thickness / 2},
        {centerX + crosshairSize / 2, centerY + thickness / 2},
        {centerX - crosshairSize / 2, centerY + thickness / 2},
        // Vertical line
        {centerX - thickness / 2, centerY - crosshairSize / 2},
        {centerX + thickness / 2, centerY - crosshairSize / 2},
        {centerX + thickness / 2, centerY + crosshairSize / 2},
        {centerX - thickness / 2, centerY + crosshairSize / 2}
    };

    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);

    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Crosshair::render() {
    // Disable depth testing for 2D UI elements
    glDisable(GL_DEPTH_TEST);

    shader_.use();
    shader_.setMat4("projection", projection_);
    shader_.setVec3("crosshairColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White color

    glBindVertexArray(VAO_);
    // We have 2 quads (horizontal and vertical lines), 4 vertices each, so 8 vertices total
    // However, we draw them as two separate quads, so 2 draw calls of 4 vertices each or use indices.
    // For simplicity with current setup, just draw all 8 vertices as two GL_QUADS (if supported)
    // or better, draw as two sets of triangles (GL_TRIANGLES, 6 indices per quad if using EBO, or 6 vertices directly per quad).
    // The current setupMesh defines 8 vertices for two quads.
    // Let's draw them as two quads (each made of 2 triangles, so 12 vertices if expanding, or 8 vertices if using GL_QUADS or GL_TRIANGLE_FAN/STRIP variants)
    // Given the vertex order in setupMesh, they form two distinct quads.
    // We can draw these as 2 * 2 triangles = 4 triangles = 12 vertices (if expanded)
    // Or if we can rely on GL_QUADS (deprecated but often available in compat profile):
    // glDrawArrays(GL_QUADS, 0, 8); 
    // For core profile, we should use triangles.
    // Let's assume the current setup is for drawing two separate quads using triangles.
    // Each quad needs 6 vertices if drawn directly (2 triangles) or 4 vertices with an EBO.
    // The current setupMesh creates 8 vertices that define two separate quads.
    // We can draw them as two separate GL_TRIANGLE_FAN or draw each quad as two GL_TRIANGLES.

    // Draw horizontal line (first 4 vertices as two triangles)
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    // Draw vertical line (next 4 vertices as two triangles)
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

    glBindVertexArray(0);

    // Re-enable depth testing if other things need it
    glEnable(GL_DEPTH_TEST);
}

void Crosshair::updateScreenSize(int screenWidth, int screenHeight) {
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    projection_ = glm::ortho(0.0f, static_cast<float>(screenWidth_),
                             0.0f, static_cast<float>(screenHeight_));
    // Optional: Re-setup mesh if its position/size is relative to screen center and needs recalculation
    // For a simple fixed-pixel-size crosshair at the center, this might not be strictly necessary
    // if the projection matrix handles the new screen size correctly.
    // However, if the crosshair's vertex definitions are in absolute pixels that need to change
    // based on screen size (e.g. to maintain relative size), then regenerate vertices and re-buffer:
    // glDeleteVertexArrays(1, &VAO_);
    // glDeleteBuffers(1, &VBO_);
    // setupMesh(); 
} 
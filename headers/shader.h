#pragma once

#include <GL/glew.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Shader {
private:
    unsigned int ID; // Program ID

    // Utility function to check shader compilation/linking errors
    void checkCompileErrors(unsigned int shader, std::string type);

public:
    // Constructor reads and builds the shader
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    // Use/activate the shader
    void use() const;
    
    // Utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    
    // Get program ID
    unsigned int getID() const { return ID; }
}; 
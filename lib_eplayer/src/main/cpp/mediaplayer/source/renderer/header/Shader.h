//
// Created by Guns Roses on 2020/4/13.
//

#ifndef EPLAYER_SHADER_H
#define EPLAYER_SHADER_H

//#include <glad/glad.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <GLES2/gl2.h>
#include "AndroidLog.h"

using namespace std;

class Shader {
public:
    GLuint ID;

    Shader(const char* vertexPath, const char* fragmentPath,bool isPath);

    void use();

    void setBool(const string& name, bool value) const;
    void setInt(const string& name, int value) const;
    void setFloat(const string& name, float value) const;
};

#endif //EPLAYER_SHADER_H

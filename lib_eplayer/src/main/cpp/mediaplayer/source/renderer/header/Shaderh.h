//
// Created by Guns Roses on 2020/4/13.
//

#ifndef EPLAYER_SHADERH_H
#define EPLAYER_SHADERH_H

//#include <glad/glad.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

class Shader {
public:
    unsigned int ID;

    Shader(const GLchar* vertexPath, const GLchar* fragmentPath);

    void use();

    void setBool(const string& name, bool value) const;
    void setInt(const string& name, int value) const;
    void setFloat(const string& name, float value) const;
};

#endif //EPLAYER_SHADERH_H

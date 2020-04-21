//
// Created by Guns Roses on 2020/4/13.
//
#include "Shader.h"

extern void createShaderProgram(const char *vShaderCode, const char *fShaderCode, unsigned int ID);


//构造函数
Shader::Shader(const char *vertexPath, const char *fragmentPath, bool isPath) {

    //前面两个参数为文件路径
    if (isPath) {
        //1.读取着色器的代码
        string vertexCode;
        string fragmentCode;
        ifstream vShaderFile;
        ifstream fShaderFile;
        //确保文件流会输出异常
        vShaderFile.exceptions(ifstream::failbit | ifstream::badbit);
        fShaderFile.exceptions(ifstream::failbit | ifstream::badbit);

        try {
            //打开文件
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            stringstream vShaderSteam, fShaderStream;

            //读取文件到流中
            //在 C++ 编程中，我们使用流提取运算符（ >> ）从文件读取信息，就像使用该运算符从键盘输入信息一样
            //唯一不同的是，在这里您使用的是 ifstream 或 fstream 对象，而不是 cin 对象
            fShaderStream << fShaderFile.rdbuf();

            //关闭文件
            vShaderFile.close();
            fShaderFile.close();
            //将流转换为字符串
            vertexCode = vShaderSteam.str();
            fragmentCode = fShaderStream.str();
        } catch (ifstream::failure) {
            cout << "读取文件失败，请检查文件是否存在" << endl;
        }
        //2.编译着色器
        createShaderProgram(vertexCode.c_str(), fragmentCode.c_str(), ID);
    } else { //前面两个参数为shader代码的字符串
        createShaderProgram(vertexPath, fragmentPath, ID);
    }

}

void Shader::use() {
    glUseProgram(ID);
}

//设置变量
void Shader::setBool(const string &name, bool value) const {
    GLint loca = glGetUniformLocation(ID, name.c_str());
    glUniform1i(loca, (int) value);
}

void Shader::setInt(const string &name, int value) const {
    GLint loca = glGetUniformLocation(ID, name.c_str());
    glUniform1i(loca, value);
}

void Shader::setFloat(const string &name, float value) const {
    GLint loca = glGetUniformLocation(ID, name.c_str());
    glUniform1f(loca, value);
}


/**
 * 编译着色器
 * @param vShaderCode
 * @param fShaderCode
 * @param ID
 */
void createShaderProgram(const char *vShaderCode, const char *fShaderCode, unsigned int ID) {

    //2.编译着色器
    GLuint vertex, fragment;
    GLint success;
    char infoLog[512];

    //顶点着色器
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        cout << "编译顶点着色器失败，错误信息：" << infoLog << endl;
    }

    //片元着色器
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        LOGE("Error compiling shader:\n%s\n", infoLog);
        cout << "编译片元着色器失败，错误信息：" << infoLog << endl;
    }

    //着色器程序
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID); //链接程序
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(ID, 512, NULL, infoLog);
        cout << "链接着色器失败，错误信息：" << infoLog << endl;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}





























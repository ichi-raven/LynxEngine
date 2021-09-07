#include <Lynx/Resources/Resources.hpp>

const std::string Lynx::Resource::Shader::genPath(const char* target)
{
    return std::string(shaderDir) + std::string(target);
}

const std::string Lynx::Resource::Model::genPath(const char* target)
{
    return std::string(modelDir) + std::string(target);
}
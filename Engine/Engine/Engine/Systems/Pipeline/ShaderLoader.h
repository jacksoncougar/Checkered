//
// Created by root on 10/1/20.
//

#ifndef ENGINE_SHADERLOADER_H
#define ENGINE_SHADERLOADER_H

#include <string>
#include <fstream>
#include <vector>
#include "../../SystemCalls.h"
#include "../Rendering/RenderingSystem.h"

#include "nlohmann/json.hpp"

#undef assert

namespace Pipeline {

    constexpr char module[] = "Pipeline";
    constexpr auto assert = Engine::assert<module>;

    class ShaderLoader {
    public:

        template<int type>
        std::unique_ptr<Rendering::Shader> load(std::string filename) {

            std::ifstream ifs(filename);
            assert(ifs.is_open(), "load shader file " + filename);

            std::vector<std::string> lines;
            std::string line;
            while (std::getline(ifs, line)) {
                lines.push_back(line + "\n");
            }

            return std::make_unique<Rendering::Shader>(type, lines);
        }
    };

    class ProgramLoader {
    public:

        std::unique_ptr<Rendering::Program> load(std::string filename) {

        std::ifstream ifs(filename);
        assert(ifs.is_open(), "load program description file " + filename);

        nlohmann::json json;
        ifs >> json;

        Pipeline::ShaderLoader loader;
        std::vector<std::unique_ptr<Rendering::Shader>> shaders;

        for(auto shader : json["shaders"]) {
            auto type_id = shader["shader"][0].get<int>();
            auto shader_filename = shader["shader"][1].get<std::string>();

            shaders.push_back(std::move( type_id == 0 ?
                              loader.load<GL_VERTEX_SHADER>(shader_filename) :
                              loader.load<GL_FRAGMENT_SHADER>(shader_filename))
            );
        }

        return std::make_unique<Rendering::Program>(std::move(shaders));
    }
};
}

#endif //ENGINE_SHADERLOADER_H
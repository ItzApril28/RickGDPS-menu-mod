#pragma once

#include <Geode/cocos/platform/CCGL.h>
#include <string_view>

namespace bv::render {

    struct PostProcessShader {
        std::string_view name;
        std::string_view vertexSource;
        std::string_view fragmentSource;
        char const* scalarUniform = nullptr;
        char const* scalarUniform2 = nullptr;
        char const* scalarUniform3 = nullptr;
        char const* scalarUniform4 = nullptr;
        char const* scalarUniform5 = nullptr;
    };

    class PostProcessRenderer final {
    public:
        PostProcessRenderer() = default;
        PostProcessRenderer(PostProcessRenderer const&) = delete;
        PostProcessRenderer& operator=(PostProcessRenderer const&) = delete;

        bool prepare(PostProcessShader const& shader, GLsizei width, GLsizei height);
        void apply(GLuint inputTexture, GLfloat scalar = 0.f, GLfloat scalar2 = 0.f,
                   GLfloat scalar3 = 0.f, GLfloat scalar4 = 0.f, GLfloat scalar5 = 0.f);
        void reset();

    private:
        bool initialize(PostProcessShader const& shader);
        void destroyResources();

        GLuint m_program = 0;
        GLint m_invResolutionUniform = -1;
        GLint m_scalarUniform = -1;
        GLint m_scalarUniform2 = -1;
        GLint m_scalarUniform3 = -1;
        GLint m_scalarUniform4 = -1;
        GLint m_scalarUniform5 = -1;
        GLsizei m_width = 0;
        GLsizei m_height = 0;
    };

} // namespace bv::render

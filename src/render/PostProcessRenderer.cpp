#include "PostProcessRenderer.hpp"

#include "FullscreenQuad.hpp"
#include "ShaderProgram.hpp"

#include <Geode/Geode.hpp>
#include <array>
#include <cassert>

using namespace geode::prelude;

namespace bv::render {

    bool PostProcessRenderer::prepare(PostProcessShader const& shader, GLsizei width, GLsizei height) {
        glActiveTexture(GL_TEXTURE0);
        if (!initialize(shader)) {
            return false;
        }
        if (width != m_width || height != m_height) {
            glUseProgram(m_program);
            glUniform2f(
                m_invResolutionUniform,
                1.f / static_cast<GLfloat>(width),
                1.f / static_cast<GLfloat>(height)
            );
            m_width = width;
            m_height = height;
        }
        return true;
    }

    bool PostProcessRenderer::initialize(PostProcessShader const& shader) {
        if (m_program != 0) {
            return true;
        }

        std::array vertexSources{shader.vertexSource};
        std::array fragmentSources{shader.fragmentSource};
        m_program = compileShaderProgram(shader.name, vertexSources, fragmentSources);
        if (m_program == 0) {
            return false;
        }

        auto const textureUniform = glGetUniformLocation(m_program, "u_texture");
        m_invResolutionUniform = glGetUniformLocation(m_program, "u_invResolution");
        m_scalarUniform =
            shader.scalarUniform ? glGetUniformLocation(m_program, shader.scalarUniform) : -1;
        m_scalarUniform2 = shader.scalarUniform2 ? glGetUniformLocation(m_program, shader.scalarUniform2) : -1;
        m_scalarUniform3 = shader.scalarUniform3 ? glGetUniformLocation(m_program, shader.scalarUniform3) : -1;
        m_scalarUniform4 = shader.scalarUniform4 ? glGetUniformLocation(m_program, shader.scalarUniform4) : -1;
        m_scalarUniform5 = shader.scalarUniform5 ? glGetUniformLocation(m_program, shader.scalarUniform5) : -1;
        if (textureUniform < 0 || m_invResolutionUniform < 0 ||
            (shader.scalarUniform && m_scalarUniform < 0) ||
            (shader.scalarUniform2 && m_scalarUniform2 < 0) ||
            (shader.scalarUniform3 && m_scalarUniform3 < 0) ||
            (shader.scalarUniform4 && m_scalarUniform4 < 0) ||
            (shader.scalarUniform5 && m_scalarUniform5 < 0)) {
            log::error("{} shader is missing required uniforms", shader.name);
            destroyResources();
            return false;
        }

        glUseProgram(m_program);
        glUniform1i(textureUniform, 0);

        return true;
    }

    void PostProcessRenderer::apply(GLuint inputTexture, GLfloat scalar, GLfloat scalar2,
                                    GLfloat scalar3, GLfloat scalar4, GLfloat scalar5) {
        assert(m_program != 0 && inputTexture != 0);

        glUseProgram(m_program);
        if (m_scalarUniform >= 0) {
            glUniform1f(m_scalarUniform, scalar);
        }
        if (m_scalarUniform2 >= 0) glUniform1f(m_scalarUniform2, scalar2);
        if (m_scalarUniform3 >= 0) glUniform1f(m_scalarUniform3, scalar3);
        if (m_scalarUniform4 >= 0) glUniform1f(m_scalarUniform4, scalar4);
        if (m_scalarUniform5 >= 0) glUniform1f(m_scalarUniform5, scalar5);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        // Post-process passes must replace every pixel. Inheriting the game's
        // blend/scissor/depth state can leave a triangular portion of the
        // destination framebuffer dark or untouched.
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        FullscreenQuad::draw();
    }

    void PostProcessRenderer::reset() {
        destroyResources();
    }

    void PostProcessRenderer::destroyResources() {
        if (m_program != 0) {
            glDeleteProgram(m_program);
            m_program = 0;
        }

        m_invResolutionUniform = -1;
        m_scalarUniform = -1;
        m_scalarUniform2 = -1;
        m_scalarUniform3 = -1;
        m_scalarUniform4 = -1;
        m_scalarUniform5 = -1;
        m_width = 0;
        m_height = 0;
    }

} // namespace bv::render

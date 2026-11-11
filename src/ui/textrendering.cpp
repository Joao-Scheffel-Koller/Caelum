// Based on http://hamelot.io/visualization/opengl-text-without-any-external-libraries/
//   and on https://github.com/rougier/freetype-gl
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "utils.h"
#include "dejavufont.h"
#include "stb_image.h"

// AI-assisted: pixel-exact text. tools/generate_font_atlas.py bakes DejaVu Sans
// Mono with FreeType hinting at every pixel size the UI uses; PrintString picks
// the size that matches the requested scale and draws it 1:1, aligned to the
// pixel grid. The old 18 px atlas above (dejavufont.h) was drawn shrunk to
// ~13.5-16 px, sampled half a texel off and at fractional positions, which
// blurred every letter. It remains as a fallback if the files are missing.
namespace {
struct AtlasGlyph { float x=0, y=0, w=0, h=0, left=0, top=0, advance=0; };
struct AtlasSize { int pixels=0; AtlasGlyph glyphs[95]; };
struct FontAtlas {
    bool loaded=false;
    int width=0, height=0;
    std::vector<AtlasSize> sizes;
};
FontAtlas g_FontAtlas;
const char* const AtlasImage="../../data/fonts/dejavu_mono_atlas.png";
const char* const AtlasMetrics="../../data/fonts/dejavu_mono_atlas.txt";

bool LoadFontAtlas(FontAtlas& atlas)
{
    std::ifstream file(AtlasMetrics);
    if (!file) return false;
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind) || kind[0] == '#') continue;
        if (kind == "atlas") in >> atlas.width >> atlas.height;
        else if (kind == "size") { atlas.sizes.push_back(AtlasSize()); in >> atlas.sizes.back().pixels; }
        else if (kind == "glyph" && !atlas.sizes.empty())
        {
            int code; AtlasGlyph g;
            in >> code >> g.x >> g.y >> g.w >> g.h >> g.left >> g.top >> g.advance;
            if (in && code >= 32 && code <= 126) atlas.sizes.back().glyphs[code - 32] = g;
        }
    }
    return atlas.width > 0 && atlas.height > 0 && !atlas.sizes.empty();
}
} // namespace

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Função definida em main.cpp

const GLchar* const textvertexshader_source = ""
"#version 330\n"
"layout (location = 0) in vec4 position;\n"
"out vec2 texCoords;\n"
"void main()\n"
"{\n"
    "gl_Position = vec4(position.xy, 0, 1);\n"
    "texCoords = position.zw;\n"
"}\n"
"\0";

const GLchar* const textfragmentshader_source = ""
"#version 330\n"
"uniform sampler2D tex;\n"
"in vec2 texCoords;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
    // AI-assisted: white labels with a dark edge remain legible inside the cockpit.
    "vec2 d = 1.0 / vec2(textureSize(tex, 0));\n"
    "float a = texture(tex, texCoords).r;\n"
    "float edge = max(a, max(max(texture(tex, texCoords+vec2(d.x,0)).r, texture(tex, texCoords-vec2(d.x,0)).r),\n"
    "                      max(texture(tex, texCoords+vec2(0,d.y)).r, texture(tex, texCoords-vec2(0,d.y)).r)));\n"
    "fragColor = vec4(vec3(a/max(edge,0.0001)), edge);\n"
"}\n"
"\0";

void TextRendering_LoadShader(const GLchar* const shader_string, GLuint shader_id)
{
    // Define o código do shader, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, NULL);

    // Compila o código do shader (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "ERROR: OpenGL compilation failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

GLuint textVAO;
GLuint textVBO;
GLuint textprogram_id;
GLuint texttexture_id;

void TextRendering_Init()
{
    GLuint sampler;

    glGenBuffers(1, &textVBO);
    glGenVertexArrays(1, &textVAO);
    glGenTextures(1, &texttexture_id);
    glGenSamplers(1, &sampler);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    GLuint textvertexshader_id = glCreateShader(GL_VERTEX_SHADER);
    TextRendering_LoadShader(textvertexshader_source, textvertexshader_id);
    glCheckError();

    GLuint textfragmentshader_id = glCreateShader(GL_FRAGMENT_SHADER);
    TextRendering_LoadShader(textfragmentshader_source, textfragmentshader_id);
    glCheckError();

    textprogram_id = CreateGpuProgram(textvertexshader_id, textfragmentshader_id);
    glLinkProgram(textprogram_id);
    glCheckError();

    GLuint texttex_uniform;
    texttex_uniform = glGetUniformLocation(textprogram_id, "tex");
    glCheckError();

    GLuint textureunit = 31;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texttexture_id);
    int imageWidth = 0, imageHeight = 0, channels = 0;
    unsigned char* pixels = LoadFontAtlas(g_FontAtlas) ?
        stbi_load(AtlasImage, &imageWidth, &imageHeight, &channels, 1) : NULL;
    if (pixels && imageWidth == g_FontAtlas.width && imageHeight == g_FontAtlas.height)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, imageWidth, imageHeight, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        g_FontAtlas.loaded = true;
    }
    else
    {
        fprintf(stderr, "WARNING: font atlas %s not loaded; using the old 18 px font.\n", AtlasImage);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, dejavufont.tex_width, dejavufont.tex_height, 0, GL_RED, GL_UNSIGNED_BYTE, dejavufont.tex_data);
    }
    if (pixels) stbi_image_free(pixels);
    glBindSampler(textureunit, sampler);
    glCheckError();

    glBindVertexArray(textVAO);

    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glCheckError();

    glUseProgram(textprogram_id);
    glUniform1i(texttex_uniform, textureunit);
    glUseProgram(0);
    glCheckError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glCheckError();
}

float textscale = 1.5f;

// AI-assisted: draws one string from the multi-size atlas, 1:1 on the pixel grid,
// with a single draw call. x, y (pen and baseline) are NDC, as before.
static void PrintStringPixelExact(GLFWwindow* window, const std::string &str, float x, float y, float scale)
{
    int windowWidth, windowHeight, width, height;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(window, &width, &height);
    if (windowWidth <= 0 || windowHeight <= 0 || width <= 0 || height <= 0) return;

    // Same on-screen size as the old renderer: 18 px font * scale * textscale / 2
    // (logical pixels), converted to framebuffer pixels for HiDPI screens.
    const float wanted = dejavufont.size * scale * textscale * 0.5f * width / windowWidth;
    const AtlasSize* best = &g_FontAtlas.sizes[0];
    for (const AtlasSize& size : g_FontAtlas.sizes)
        if (std::fabs(size.pixels - wanted) < std::fabs(best->pixels - wanted)) best = &size;
    // Within half a pixel: draw 1:1 (the size changes by at most 0.5 px).
    // Only titles beyond the largest baked sizes are scaled, and only slightly.
    const float k = std::fabs(best->pixels - wanted) <= 0.5f ? 1.0f : wanted / best->pixels;

    // Pen and baseline on whole framebuffer pixels (pixel rows grow upward here).
    float pen = std::floor((x + 1.0f) * 0.5f * width + 0.5f);
    const float baseline = std::floor((y + 1.0f) * 0.5f * height + 0.5f);

    std::vector<float> vertices;
    vertices.reserve(str.size() * 24);
    for (size_t i = 0; i < str.size(); i++)
    {
        const int index = (int)(unsigned char)str[i] - 32;
        if (index < 0 || index >= 95) continue;
        const AtlasGlyph& g = best->glyphs[index];
        if (g.w > 0 && g.h > 0)
        {
            // One empty texel around the ink (the atlas keeps 2) so the dark
            // outline drawn by the fragment shader is not clipped by the quad.
            const float e = 1.0f;
            const float px0 = std::floor(pen + 0.5f) + (g.left - e) * k;
            const float py0 = baseline + (g.top + e) * k;     // Top edge.
            const float px1 = px0 + (g.w + 2 * e) * k, py1 = py0 - (g.h + 2 * e) * k;
            // Texel edges map onto pixel edges: no half-texel shift.
            const float s0 = (g.x - e) / g_FontAtlas.width, s1 = (g.x + g.w + e) / g_FontAtlas.width;
            const float t0 = (g.y - e) / g_FontAtlas.height, t1 = (g.y + g.h + e) / g_FontAtlas.height;
            const float X0 = px0 / width * 2.0f - 1.0f, X1 = px1 / width * 2.0f - 1.0f;
            const float Y0 = py0 / height * 2.0f - 1.0f, Y1 = py1 / height * 2.0f - 1.0f;
            const float quad[24] = {
                X0, Y0, s0, t0,  X0, Y1, s0, t1,  X1, Y1, s1, t1,
                X0, Y0, s0, t0,  X1, Y1, s1, t1,  X1, Y0, s1, t0 };
            vertices.insert(vertices.end(), quad, quad + 24);
        }
        // Monospace: whole-pixel advance at 1:1 keeps the spacing even.
        pen += k == 1.0f ? std::floor(g.advance + 0.5f) : g.advance * k;
    }
    if (vertices.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthFunc(GL_ALWAYS);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(textprogram_id);
    glBindVertexArray(textVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));
    glBindVertexArray(0);
    glUseProgram(0);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f)
{
    if (g_FontAtlas.loaded) { PrintStringPixelExact(window, str, x, y, scale); return; }
    scale *= textscale;
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float sx = scale / width;
    float sy = scale / height;

    for (size_t i = 0; i < str.size(); i++)
    {
        // Find the glyph for the character we are looking for
        texture_glyph_t *glyph = 0;
        for (size_t j = 0; j < dejavufont.glyphs_count; ++j)
        {
            if (dejavufont.glyphs[j].codepoint == (uint32_t)str[i])
            {
                glyph = &dejavufont.glyphs[j];
                break;
            }
        }
        if (!glyph) {
            continue;
        }
        x += glyph->kerning[0].kerning;
        float x0 = (float) (x + glyph->offset_x * sx);
        float y0 = (float) (y + glyph->offset_y * sy);
        float x1 = (float) (x0 + glyph->width * sx);
        float y1 = (float) (y0 - glyph->height * sy);

        float s0 = glyph->s0 - 0.5f/dejavufont.tex_width;
        float t0 = glyph->t0 - 0.5f/dejavufont.tex_height;
        float s1 = glyph->s1 - 0.5f/dejavufont.tex_width;
        float t1 = glyph->t1 - 0.5f/dejavufont.tex_height;

        struct {float x, y, s, t;} data[6] = {
            { x0, y0, s0, t0 },
            { x0, y1, s0, t1 },
            { x1, y1, s1, t1 },
            { x0, y0, s0, t0 },
            { x1, y1, s1, t1 },
            { x1, y0, s1, t0 }
        };

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDepthFunc(GL_ALWAYS);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 24 * sizeof(float), data);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glUseProgram(textprogram_id);
        glBindVertexArray(textVAO);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
        glUseProgram(0);
        glDepthFunc(GL_LESS);

        glDisable(GL_BLEND);

        x += (glyph->advance_x * sx);
    }
}

float TextRendering_LineHeight(GLFWwindow* window)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    return dejavufont.height / height * textscale;
}

float TextRendering_CharWidth(GLFWwindow* window)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    return dejavufont.glyphs[32].advance_x / width * textscale;
}

void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f)
{
    char buffer[40];
    float lineheight = TextRendering_LineHeight(window) * scale;

    snprintf(buffer, 40, "[%+0.2f %+0.2f %+0.2f %+0.2f]", M[0][0], M[1][0], M[2][0], M[3][0]);
    TextRendering_PrintString(window, buffer, x, y, scale);
    snprintf(buffer, 40, "[%+0.2f %+0.2f %+0.2f %+0.2f]", M[0][1], M[1][1], M[2][1], M[3][1]);
    TextRendering_PrintString(window, buffer, x, y - lineheight, scale);
    snprintf(buffer, 40, "[%+0.2f %+0.2f %+0.2f %+0.2f]", M[0][2], M[1][2], M[2][2], M[3][2]);
    TextRendering_PrintString(window, buffer, x, y - 2*lineheight, scale);
    snprintf(buffer, 40, "[%+0.2f %+0.2f %+0.2f %+0.2f]", M[0][3], M[1][3], M[2][3], M[3][3]);
    TextRendering_PrintString(window, buffer, x, y - 3*lineheight, scale);
}

void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f)
{
    char buffer[10];
    float lineheight = TextRendering_LineHeight(window) * scale;

    snprintf(buffer, 10, "[%+0.2f]", v.x);
    TextRendering_PrintString(window, buffer, x, y, scale);
    snprintf(buffer, 10, "[%+0.2f]", v.y);
    TextRendering_PrintString(window, buffer, x, y - lineheight, scale);
    snprintf(buffer, 10, "[%+0.2f]", v.z);
    TextRendering_PrintString(window, buffer, x, y - 2*lineheight, scale);
    snprintf(buffer, 10, "[%+0.2f]", v.w);
    TextRendering_PrintString(window, buffer, x, y - 3*lineheight, scale);
}

void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f)
{
    char buffer[70];
    float lineheight = TextRendering_LineHeight(window) * scale;

    auto r = M*v;
    snprintf(buffer, 70, "[%+0.2f %+0.2f %+0.2f %+0.2f][%+0.2f]     [%+0.2f]\n", M[0][0], M[1][0], M[2][0], M[3][0], v[0], r[0]);
    TextRendering_PrintString(window, buffer, x, y, scale);
    snprintf(buffer, 70, "[%+0.2f %+0.2f %+0.2f %+0.2f][%+0.2f]     [%+0.2f]\n", M[0][1], M[1][1], M[2][1], M[3][1], v[1], r[1]);
    TextRendering_PrintString(window, buffer, x, y - lineheight, scale);
    snprintf(buffer, 70, "[%+0.2f %+0.2f %+0.2f %+0.2f][%+0.2f] --> [%+0.2f]\n", M[0][2], M[1][2], M[2][2], M[3][2], v[2], r[2]);
    TextRendering_PrintString(window, buffer, x, y - 2*lineheight, scale);
    snprintf(buffer, 70, "[%+0.2f %+0.2f %+0.2f %+0.2f][%+0.2f]     [%+0.2f]\n", M[0][3], M[1][3], M[2][3], M[3][3], v[3], r[3]);
    TextRendering_PrintString(window, buffer, x, y - 3*lineheight, scale);
}

void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f)
{
    char buffer[70];
    float lineheight = TextRendering_LineHeight(window) * scale;

    auto r = M*v;
    snprintf(buffer, 70, "[%5.1f %5.1f %5.1f %5.1f][%5.2f]     [%+6.1f]\n", M[0][0], M[1][0], M[2][0], M[3][0], v[0], r[0]);
    TextRendering_PrintString(window, buffer, x, y, scale);
    snprintf(buffer, 70, "[%5.1f %5.1f %5.1f %5.1f][%5.2f]     [%+6.1f]\n", M[0][1], M[1][1], M[2][1], M[3][1], v[1], r[1]);
    TextRendering_PrintString(window, buffer, x, y - lineheight, scale);
    snprintf(buffer, 70, "[%5.1f %5.1f %5.1f %5.1f][%5.2f] --> [%+6.1f]\n", M[0][2], M[1][2], M[2][2], M[3][2], v[2], r[2]);
    TextRendering_PrintString(window, buffer, x, y - 2*lineheight, scale);
    snprintf(buffer, 70, "[%5.1f %5.1f %5.1f %5.1f][%5.2f]     [%+6.1f]\n", M[0][3], M[1][3], M[2][3], M[3][3], v[3], r[3]);
    TextRendering_PrintString(window, buffer, x, y - 3*lineheight, scale);
}

void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f)
{
    auto r = M*v;
    auto w = r[3];

    char buffer[90];
    float lineheight = TextRendering_LineHeight(window) * scale;

    snprintf(buffer, 90, "[%+0.2f %+0.2f %+0.2f %+0.2f][%+0.2f]     [%+0.2f]        [%+0.2f]\n", M[0][0], M[1][0], M[2][0], M[3][0], v[0], r[0], r[0]/w);
    TextRendering_PrintString(window, buffer, x, y, scale);
    snprintf(buffer, 90, "[%+0.2f %+0.2f %+0.2f %+0.2f][%+0.2f]     [%+0.2f] div. w [%+0.2f]\n", M[0][1], M[1][1], M[2][1], M[3][1], v[1], r[1], r[1]/w);
    TextRendering_PrintString(window, buffer, x, y - lineheight, scale);
    snprintf(buffer, 90, "[%+0.2f %+0.2f %+0.2f %+0.2f][%+0.2f] --> [%+0.2f] -----> [%+0.2f]\n", M[0][2], M[1][2], M[2][2], M[3][2], v[2], r[2], r[2]/w);
    TextRendering_PrintString(window, buffer, x, y - 2*lineheight, scale);
    snprintf(buffer, 90, "[%+0.2f %+0.2f %+0.2f %+0.2f][%+0.2f]     [%+0.2f]        [%+0.2f]\n", M[0][3], M[1][3], M[2][3], M[3][3], v[3], r[3], r[3]/w);
    TextRendering_PrintString(window, buffer, x, y - 3*lineheight, scale);
}

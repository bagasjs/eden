#include "ren.h"


/// TODO: remove some of this dependencies, especially glad.h maybe define our own functions?
#include "tp/glad.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

typedef struct { float x, y;       } V2f;
typedef struct { float x, y, z;    } V3f;
typedef struct { float x, y, z, w; } V4f;
#define v2f(X, Y)       (V2f){ .x=(X), .y=(Y) }
#define v3f(X, Y, Z)    (V3f){ .x=(X), .y=(Y), .z=(Z) }
#define v4f(X, Y, Z, W) (V4f){ .x=(X), .y=(Y), .z=(Z), .w=(W) }

typedef struct {
    V3f   pos;
    V4f   color;
    V2f   uv;
    float tex;
} RenVertex;

struct RenImage {
    GLuint texture;
    uint8_t *pixels;
    int width;
    int height;
    int n_channels;
};

#define REN_MAX_VERTICES (1024)
#define REN_MAX_ELEMENTS (1024)
#define REN_MAX_TEXTURES (8)
static const char *vert_src = 
    "#version 330\n"
    "layout(location=0) in vec3  a_position;\n"
    "layout(location=1) in vec4  a_color;\n"
    "layout(location=2) in vec2  a_uv;\n"
    "layout(location=3) in float a_tex;\n"
    "out vec4  v_color;\n"
    "out vec2  v_uv;\n"
    "out float v_tex;\n"
    "uniform mat4 u_proj;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = u_proj * vec4(a_position, 1.0);\n"
    "    v_uv = a_uv;\n"
    "    v_color = a_color;\n"
    "    v_tex = a_tex;\n"
    "}\n";

static const char *frag_src = 
    "#version 330\n"
    "layout(location=0) out vec4 o_color;\n"
    "uniform sampler2D u_textures[8];\n"
    "in vec4 v_color;\n"
    "in vec2 v_uv;\n"
    "in float v_tex;\n"
    "void main()\n"
    "{\n"
    "    int index = int(v_tex);\n"
    "    if(index == 1) {\n"
    "        o_color = texture(u_textures[1], v_uv) * v_color;\n"
    "    } else if(index == 2) {\n"
    "        o_color = texture(u_textures[2], v_uv) * v_color;\n"
    "    } else if(index == 3) {\n"
    "        o_color = texture(u_textures[3], v_uv) * v_color;\n"
    "    } else if(index == 4) {\n"
    "        o_color = texture(u_textures[4], v_uv) * v_color;\n"
    "    } else if(index == 5) {\n"
    "        o_color = texture(u_textures[5], v_uv) * v_color;\n"
    "    } else if(index == 6) {\n"
    "        o_color = texture(u_textures[6], v_uv) * v_color;\n"
    "    } else if(index == 7) {\n"
    "        o_color = texture(u_textures[7], v_uv) * v_color;\n"
    "    } else {\n"
    "        o_color = v_color;\n"
    "    }\n"
    "}\n";

struct {
    GLuint vao, vbo, ibo;
    GLuint shader_program;

    GLint textures_uniform_loc;
    GLint proj_mat_uniform_loc;

    RenVertex vertices[REN_MAX_VERTICES];
    GLuint    elements[REN_MAX_ELEMENTS];
    GLuint    count_vertices;
    GLuint    count_elements;

    GLuint textures[REN_MAX_TEXTURES];
    int    texture_samplers[REN_MAX_TEXTURES];
    GLuint count_textures;

    GLfloat proj[16];
} ren = {0};

static void debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;
    fprintf(stderr, "[ERROR] OPENGL: (%d)(%d) %s\n", severity, type, message);
}

bool ren_init(void)
{
    memset(&ren, 0, sizeof(ren));

    glDebugMessageCallback(debug_message_callback, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenVertexArrays(1, &ren.vao);
    glBindVertexArray(ren.vao);

    glGenBuffers(1, &ren.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ren.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ren.vertices), ren.vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenVertex), (GLvoid*)offsetof(RenVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(RenVertex), (GLvoid*)offsetof(RenVertex, color));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RenVertex), (GLvoid*)offsetof(RenVertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(RenVertex), (GLvoid*)offsetof(RenVertex, tex));

    glGenBuffers(1, &ren.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ren.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ren.elements), ren.elements, GL_DYNAMIC_DRAW);

    GLuint vert_shader, frag_shader;
    GLint success;
    GLchar info_log[512];

    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, (const GLchar* const*)&vert_src, NULL);
    glCompileShader(vert_shader);
    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vert_shader, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "[ERROR] Vertex shader compilation error\n%s\n", info_log);
        return false;
    }

    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, (const GLchar* const*)&frag_src, NULL);
    glCompileShader(frag_shader);
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(frag_shader, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "[ERROR] Fragment shader compilation error\n%s\n", info_log);
        return false;
    }

    ren.shader_program = glCreateProgram();
    glAttachShader(ren.shader_program, vert_shader);
    glAttachShader(ren.shader_program, frag_shader);
    glLinkProgram(ren.shader_program);
    glGetProgramiv(ren.shader_program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(ren.shader_program, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "[ERROR] Shader linking error\n%s\n", info_log);
        return false;
    }
    glUseProgram(ren.shader_program);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    ren.textures_uniform_loc = glGetUniformLocation(ren.shader_program, "u_textures");
    if(ren.textures_uniform_loc < 0) {
        fprintf(stderr, "[ERROR] Could not find texture samplers uniform in shaders\n");
        return false;
    }

    ren.proj_mat_uniform_loc = glGetUniformLocation(ren.shader_program, "u_proj");
    if(ren.proj_mat_uniform_loc < 0) {
        fprintf(stderr, "[ERROR] Could not find projection matrix uniform in shaders\n");
        return false;
    }

    return true;
}

void ren_deinit(void)
{
    glDeleteProgram(ren.shader_program);
    glDeleteBuffers(1, &ren.vbo);
    glDeleteBuffers(1, &ren.ibo);
    glDeleteVertexArrays(1, &ren.vao);
}

void ren_viewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);

    float l = x;
    float r = width;
    float t = y;
    float d = height;
    float n = 0.0f;
    float f = 100.0f;

    float lr = 1.0f / (l - r);
    float dt = 1.0f / (d - t);
    float nf = 1.0f / (n - f);

    ren.proj[0] = -2.0f * lr;
    ren.proj[1] = 0.0f;
    ren.proj[2] = 0.0f;
    ren.proj[3] = 0.0f;
    ren.proj[4] = 0.0f;
    ren.proj[5] = -2.0f * dt;
    ren.proj[6] = 0.0f;
    ren.proj[7] = 0.0f;
    ren.proj[8] = 0.0f;
    ren.proj[9] = 0.0f;
    ren.proj[10] = 2.0f * nf;
    ren.proj[11] = 0.0f;
    ren.proj[12] = (l + r) * lr;
    ren.proj[13] = (t + d) * dt;
    ren.proj[14] = (n + f) * nf;
    ren.proj[15] = 1.0f;
}

void ren_clear(RenColor color)
{
    glClearColor(
            (float)color.r/255,
            (float)color.g/255,
            (float)color.b/255,
            (float)color.a/255);
    glClear(GL_COLOR_BUFFER_BIT);
}

RenImage *ren_load_image(const void *pixels, int width, int height, int n_channels)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);

    GLenum internal_format = GL_RGBA8;
    GLenum data_format = n_channels == 4 ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    RenImage *image = MALLOC_WITH_LABEL(sizeof(*image) + (width * height * n_channels), "RenImage");
    image->texture = texture;
    image->width   = width;
    image->height  = height;
    image->n_channels = n_channels;
    image->pixels = (void*)(image + 1);
    memcpy(image + 1, pixels, width * height * n_channels);
    return image;
}

void ren_unload_image(RenImage *image)
{
    glDeleteTextures(1, &image->texture);
    FREE_WITH_LABEL(image, "RenImage");
}

RenRect ren_get_image_rect(RenImage *image)
{
    return (RenRect) {
        .x = 0,
        .y = 0,
        .w = image->width,
        .h = image->height,
    };
}

void ren_flush(void)
{
    glBindBuffer(GL_ARRAY_BUFFER, ren.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, ren.count_vertices*sizeof(*ren.vertices), ren.vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ren.ibo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, ren.count_elements*sizeof(*ren.elements), ren.elements);

    glBindVertexArray(ren.vao);
    glUseProgram(ren.shader_program);
    for(uint32_t i = 0; i < REN_MAX_TEXTURES; ++i) {
        if(ren.texture_samplers[i] != 0) {
            glActiveTexture(GL_TEXTURE0 + ren.texture_samplers[i]);
            glBindTexture(GL_TEXTURE_2D,  ren.textures[i]);
        }
    }

    glUniform1iv(ren.textures_uniform_loc, REN_MAX_TEXTURES, ren.texture_samplers);
    glUniformMatrix4fv(ren.proj_mat_uniform_loc, 1, GL_FALSE, ren.proj);
    glDrawElements(GL_TRIANGLES, ren.count_elements, GL_UNSIGNED_INT, 0);

    ren.count_elements = 0;
    ren.count_vertices = 0;

    // Texture stub
    ren.textures[0] = -1;
    ren.count_textures = 1;
}

void quad(
        V2f p0, V4f c0, V2f uv0, float tex0,
        V2f p1, V4f c1, V2f uv1, float tex1,
        V2f p2, V4f c2, V2f uv2, float tex2,
        V2f p3, V4f c3, V2f uv3, float tex3)
{
    ren.elements[ren.count_elements + 0] = ren.count_vertices + 0;
    ren.elements[ren.count_elements + 1] = ren.count_vertices + 1;
    ren.elements[ren.count_elements + 2] = ren.count_vertices + 2;
    ren.elements[ren.count_elements + 3] = ren.count_vertices + 2;
    ren.elements[ren.count_elements + 4] = ren.count_vertices + 3;
    ren.elements[ren.count_elements + 5] = ren.count_vertices + 0;
    ren.count_elements += 6;

    ren.vertices[ren.count_vertices + 0].pos = v3f(p0.x, p0.y, 0.0f);
    ren.vertices[ren.count_vertices + 0].color = c0;
    ren.vertices[ren.count_vertices + 0].uv = uv0;
    ren.vertices[ren.count_vertices + 0].tex = tex0;
    ren.vertices[ren.count_vertices + 1].pos = v3f(p1.x, p1.y, 0.0f);
    ren.vertices[ren.count_vertices + 1].color = c1;
    ren.vertices[ren.count_vertices + 1].uv = uv1;
    ren.vertices[ren.count_vertices + 1].tex = tex1;
    ren.vertices[ren.count_vertices + 2].pos = v3f(p2.x, p2.y, 0.0f);
    ren.vertices[ren.count_vertices + 2].color = c2;
    ren.vertices[ren.count_vertices + 2].uv = uv2;
    ren.vertices[ren.count_vertices + 2].tex = tex2;
    ren.vertices[ren.count_vertices + 3].pos = v3f(p3.x, p3.y, 0.0f);
    ren.vertices[ren.count_vertices + 3].color = c3;
    ren.vertices[ren.count_vertices + 3].uv = uv3;
    ren.vertices[ren.count_vertices + 3].tex = tex3;
    ren.count_vertices += 4;
}

#define color2v4f(C) (V4f){ .x=(float)(C).r/255, .y=(float)(C).g/255, .z=(float)(C).b/255, .w=(float)(C).a/255, }

void ren_draw_rect(RenRect r, RenColor color)
{
    V4f c = color2v4f(color);
    V2f z = v2f(0.0f, 0.0f);
    quad(v2f(r.x      , r.y      ), c, z, 0.0f,
         v2f(r.x + r.w, r.y      ), c, z, 0.0f,
         v2f(r.x + r.w, r.y + r.h), c, z, 0.0f,
         v2f(r.x      , r.y + r.h), c, z, 0.0f);
}

static int find_image_slot_or_create_new_one(RenImage *image)
{
    for(int i = 0; i < REN_MAX_TEXTURES; ++i) {
        if(ren.textures[i] == image->texture) {
            return i;
        }
    }
    if(ren.count_textures >= REN_MAX_TEXTURES) 
        return -1;
    int slot = ren.count_textures;
    ren.textures[slot] = image->texture;
    ren.texture_samplers[slot] = slot;
    ren.count_textures += 1;
    return slot;
}

void ren_draw_image(RenImage *image, RenRect src, RenRect dst, RenColor color)
{
    V4f c = color2v4f(color);
    int slot = find_image_slot_or_create_new_one(image);
    if(slot < 0) {
        // That means we're out of slots, we need to flush
        ren_flush();
        slot = find_image_slot_or_create_new_one(image);
        assert(slot > 0);
    }

    float uv_l = (float)src.x/image->width;
    float uv_t = (float)src.y/image->height;
    float uv_r = uv_l + (float)src.w/image->width;
    float uv_b = uv_t + (float)src.h/image->height;

    quad(v2f(dst.x         , dst.y        ), c, v2f(uv_l, uv_t), slot,
         v2f(dst.x + dst.w , dst.y        ), c, v2f(uv_r, uv_t), slot,
         v2f(dst.x + dst.w , dst.y + dst.h), c, v2f(uv_r, uv_b), slot,
         v2f(dst.x         , dst.y + dst.h), c, v2f(uv_l, uv_b), slot);
}

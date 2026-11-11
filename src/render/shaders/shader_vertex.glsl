#version 330 core

// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_position;

// AI-assisted: wind on the tree billboards (docs/wind.md). Same formula as
// WindOffset() in src/render/WindSway.cpp. wind_strength is 1 only for trees.
uniform float wind_strength;
uniform float wind_time;
uniform vec2  wind_direction;
uniform float wind_amplitude;
uniform float wind_angular_frequency;
uniform float wind_wavenumber;
uniform float wind_flutter;
uniform float wind_flutter_frequency;

// Atributos de vértice que serão gerados como saída ("out") pelo Vertex Shader.
// ** Estes serão interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais serão recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;

void main()
{
    // A variável gl_Position define a posição final de cada vértice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC), onde cada
    // coeficiente estará entre -1 e 1 após divisão por w.
    // Veja {+NDC2+}.
    //
    // O código em "main.cpp" define os vértices dos modelos em coordenadas
    // locais de cada modelo (array model_coefficients). Abaixo, utilizamos
    // operações de modelagem, definição da câmera, e projeção, para computar
    // as coordenadas finais em NDC (variável gl_Position). Após a execução
    // deste Vertex Shader, a placa de vídeo (GPU) fará a divisão por W. Veja
    // slides 41-67 e 69-86 do documento Aula_09_Projecoes.pdf.

    vec4 world = model * model_coefficients;
    if (wind_strength > 0.0)
    {
        // Texture v of the billboard: 0 at the base (fixed), 1 at the top.
        float v = clamp(texture_coefficients.y, 0.0, 1.0);
        float bend = v * v;
        // Gust travelling with the wind: phase from the position along it.
        float phase = -wind_wavenumber * dot(world.xz, wind_direction);
        float sway = 0.7 * sin(wind_angular_frequency * wind_time + phase)
                   + 0.3 * sin(2.3 * wind_angular_frequency * wind_time + 1.7 * phase);
        // Leaf rustle on the crown only.
        float crown = clamp((v - 0.4) / 0.6, 0.0, 1.0);
        float rustle = sin(wind_flutter_frequency * wind_time + 0.37 * world.x + 0.71 * world.z);
        float along = wind_strength * (wind_amplitude * bend * sway + wind_flutter * crown * rustle);
        world.xz += wind_direction * along;
    }
    // Subtract the camera before rotating. Avoid cancellation between large
    // map coordinates and view translation when surfaces are millimeters apart.
    vec4 camera_relative = vec4(mat3(view)*(world.xyz-camera_position),1.0);
    gl_Position = projection * camera_relative;

    // Como as variáveis acima  (tipo vec4) são vetores com 4 coeficientes,
    // também é possível acessar e modificar cada coeficiente de maneira
    // independente. Esses são indexados pelos nomes x, y, z, e w (nessa
    // ordem, isto é, 'x' é o primeiro coeficiente, 'y' é o segundo, ...):
    //
    //     gl_Position.x = model_coefficients.x;
    //     gl_Position.y = model_coefficients.y;
    //     gl_Position.z = model_coefficients.z;
    //     gl_Position.w = model_coefficients.w;
    //

    // Agora definimos outros atributos dos vértices que serão interpolados pelo
    // rasterizador para gerar atributos únicos para cada fragmento gerado.

    // Posição do vértice atual no sistema de coordenadas global (World).
    position_world = world;

    // Posição do vértice atual no sistema de coordenadas local do modelo.
    position_model = model_coefficients;

    // Normal do vértice atual no sistema de coordenadas global (World).
    // Veja slides 123-151 do documento Aula_07_Transformacoes_Geometricas_3D.pdf.
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    // Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
    texcoords = texture_coefficients;
}


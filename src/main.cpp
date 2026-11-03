//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

//Newly definied classes
#include "aircraft/Aircraft.hpp"
#include "aircraft/FlightModel.hpp"
#include "aircraft/AircraftCatalog.hpp"
#include "render/GameScene.hpp"
#include "camera/CameraInput.hpp"
#include "platform/FrameLimiter.hpp"
#include "render/RenderInspection.hpp"
#include "render/AircraftLightsInspection.hpp"
#include "ui/FlightHud.hpp"
#include "ui/HudInspection.hpp"
#include "collision/CollisionDebug.hpp"
#include "mission/FlightMission.hpp"
#include "mission/MissionInspection.hpp"
#include "ui/MissionOverlay.hpp"
#include "ui/MissionMarker.hpp"
#include "ui/FrontendModel.hpp"
#include "ui/FrontendScreen.hpp"
#include "ui/FrontendInspection.hpp"
#include <memory>

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void FocusCallback(GLFWwindow* window, int focused);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// Camera math/state and its GLFW input adapter are independent of flight physics.
CameraController g_Camera;
CameraFrame g_CameraFrame;
std::unique_ptr<CameraInput> g_CameraInput;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;
bool g_ShowCollisionVolumes=false,g_ResetFlightRequested=false,g_PlayerCrashed=false;
bool g_ShowMissionMarker=false;
std::unique_ptr<frontend::FrontendModel> g_Frontend;
frontend::Action g_MenuAction=frontend::Action::None;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Avião global

Aircraft MainAircraft;
glm::vec3 velocity = glm::vec3(0.00000008f, 0.00000008f, 0.00000008f);

FlightModel MainModel;


int main(int argc, char* argv[])
{
    // Optional developer check: load the real scene, render 60 frames, then exit.
    const bool collision_check=argc>2&&std::string(argv[1])=="--collision-check";
    const bool mission_check=argc>2&&std::string(argv[1])=="--mission-check";
    const bool menu_check=argc>2&&std::string(argv[1])=="--menu-check";
    const bool lights_check=argc>2&&std::string(argv[1])=="--lights-check";
    bool menuInspectionActive=menu_check;
    FrontendInspection frontendInspection(menu_check?argv[2]:"");
    bool missionInspectionActive=mission_check;
    MissionInspection missionInspection(mission_check?argv[2]:"");
    const bool smoke_test = collision_check||(argc > 1 && std::string(argv[1]) == "--smoke-test");
    const bool texture_check=argc>2 && std::string(argv[1])=="--texture-check";
    const bool hud_check=argc>2 && std::string(argv[1])=="--hud-check";
    RenderInspection inspection(texture_check?argv[2]:"");
    HudInspection hudInspection(hud_check?argv[2]:"");
    bool keep_open=false;
    bool hiddenWindow=lights_check;
    std::size_t initialAircraft=0;
    std::string loadingCapturePrefix;
    for(int i=1;i<argc;++i) {
        if(std::string(argv[i])=="--keep-open")keep_open=true;
        if(std::string(argv[i])=="--hidden")hiddenWindow=true;
        // Explicit opt-in for repeatable GPU inspections. Normal play starts hidden.
        if(std::string(argv[i])=="--show-mission-marker")g_ShowMissionMarker=true;
        if(std::string(argv[i])=="--aircraft=light")initialAircraft=1;
        const std::string arg=argv[i],captureFlag="--capture-loading=";
        if(arg.compare(0,captureFlag.size(),captureFlag)==0)loadingCapturePrefix=arg.substr(captureFlag.size());
    }
    int rendered_frames = 0;
    int exit_code = EXIT_SUCCESS;
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES,4);
    if(hiddenWindow)glfwWindowHint(GLFW_VISIBLE,GL_FALSE);

    // Start in a resizable 720p window.
    GLFWwindow* window;
    window = glfwCreateWindow(1280, 720, "Caelum - Rio de Janeiro", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetWindowFocusCallback(window, FocusCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);
    // Software pacing below caps at60 even on 120/144/165Hz monitors.
    glfwSwapInterval(0);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    int initialWidth,initialHeight;
    glfwGetFramebufferSize(window,&initialWidth,&initialHeight);
    FramebufferSizeCallback(window,initialWidth,initialHeight);

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Isolated GPU check: both real aircraft, without reading the entire city.
    // The function releases all of its GL objects before this context ends.
    if(lights_check) {
        int result=EXIT_SUCCESS;
        try {InspectAircraftLights(window,g_GpuProgramID,argv[2]);}
        catch(const std::exception& error) {
            std::fprintf(stderr,"Aircraft lights inspection failed: %s\n",error.what());result=EXIT_FAILURE;
        }
        glDeleteProgram(g_GpuProgramID);g_GpuProgramID=0;
        glfwDestroyWindow(window);glfwTerminate();return result;
    }

    // The front end owns no scene data: the menu appears before any OBJ loads.
    TextRendering_Init();
    std::unique_ptr<frontend::FrontendScreen> frontendScreen(new frontend::FrontendScreen);
    std::vector<frontend::AircraftChoice> choices;
    for(const auto& spec:AircraftCatalog()) {
        std::ifstream model(spec.modelPath);
        choices.push_back({spec.id,spec.name,spec.description,bool(model)});
    }
    g_Frontend.reset(new frontend::FrontendModel(choices));
    std::unique_ptr<GameScene> scene;
    bool sceneReady=false;
    std::size_t loadAttempt=0;
    std::string loadedAircraft;
    mission::FlightMission flightMission;
    MissionOverlay missionOverlay;
    std::unique_ptr<FlightHud> hud(new FlightHud);
    std::unique_ptr<CollisionDebug> collisionDebug(new CollisionDebug);
    std::unique_ptr<MissionMarker> missionMarker(new MissionMarker);
    double previous_time=glfwGetTime(),smoke_start=previous_time;
    FrameLimiter frameLimiter(60.0);
    const bool inspectionRun=smoke_test||texture_check||hud_check||mission_check;
    const auto startFlight=[&](std::size_t index)->bool {
        const std::size_t attempt=++loadAttempt;
        const auto& spec=AircraftCatalog().at(index);
        if(g_CameraInput)g_CameraInput->Focus(false);
        g_MenuAction=frontend::Action::None;
        g_Frontend->BeginLoading();
        const bool hadFlight=sceneReady;
        bool cancelRequested=false;
        LoadProgress progress("Preparando voo",0,1);
        const double loadingStarted=glfwGetTime();
        double nextLoadingDraw=0;
        int capturedLoading=0;
        LoadCallbacks callbacks;
        callbacks.pump=[&] {
            glfwPollEvents();
            if(g_MenuAction==frontend::Action::CancelLoading) {
                cancelRequested=true;nextLoadingDraw=0;g_MenuAction=frontend::Action::None;
            }
            int fw,fh;glfwGetFramebufferSize(window,&fw,&fh);
            const double loadingNow=glfwGetTime();
            if(fw>0&&fh>0&&loadingNow>=nextLoadingDraw) {
                nextLoadingDraw=loadingNow+1.0/60;
                const double fraction=progress.total?double(progress.completed)/progress.total:0;
                frontendScreen->DrawLoading(window,fraction,cancelRequested?"Cancelando":progress.stage,
                    cancelRequested?"Voltando ao menu...":spec.name+" | Santos Dumont -> Galeao",glfwGetTime()-loadingStarted);
                if(!loadingCapturePrefix.empty()&&capturedLoading<3&&fraction>=capturedLoading*.5) {
                    CaptureFrontendFrame(window,loadingCapturePrefix+"-"+std::to_string(attempt)+"-"+
                                         std::to_string(capturedLoading)+".ppm");
                    ++capturedLoading;
                }
                glfwSwapBuffers(window);
            }
            if(glfwWindowShouldClose(window)||cancelRequested)throw LoadCancelled();
        };
        callbacks.progress=[&](const LoadProgress& next) {progress=next;nextLoadingDraw=0;callbacks.pump();};
        try {
            callbacks.pump();
            if(!sceneReady) {
                scene.reset(new GameScene);
                scene->Load(window,callbacks,spec.modelPath,spec.cockpitEye);
                sceneReady=true;
            } else if(loadedAircraft!=spec.id) {
                scene->LoadAircraft(spec.modelPath,spec.cockpitEye,callbacks);
            }
            loadedAircraft=spec.id;
            scene->SetAirframeType(spec.light?collision::AirframeType::LightTaildragger:collision::AirframeType::Airliner);
            scene->SetLightProfile(spec.id);
            scene->ResetPlayer(MainAircraft);g_PlayerCrashed=false;g_ResetFlightRequested=false;
            flightMission.Reset();flightMission.Update(scene->PlayerMissionSample(MainAircraft),0);
            std::printf("Flight ready: %s state=%d gear=%u origin_armed=%d reused_scene=%d\n",spec.id.c_str(),
                int(scene->PlayerContact().state),scene->PlayerContact().gearContacts,
                int(flightMission.Snapshot().originArmed),int(hadFlight));std::fflush(stdout);
            g_CameraFrame=scene->Cameras(MainAircraft);
            g_Camera.SetMode(CameraType::LOOK_AT,g_CameraFrame);g_Camera.Reset(g_CameraFrame);
            if(!g_CameraInput)g_CameraInput.reset(new CameraInput(window,g_Camera,g_CameraFrame));
            g_Frontend->Close();
            glfwSetWindowTitle(window,("Caelum - "+spec.name+" - Rio de Janeiro").c_str());
            previous_time=smoke_start=glfwGetTime();
            return true;
        } catch(const LoadCancelled&) {
            if(!sceneReady)scene.reset();
            if(hadFlight) {
                for(std::size_t old=0;old<AircraftCatalog().size();++old)if(AircraftCatalog()[old].id==loadedAircraft)
                    SelectFrontendAircraft(*g_Frontend,old,1280,720);
                g_Frontend->OpenPause();
            } else g_Frontend->OpenMain();
            g_MenuAction=frontend::Action::None;
        } catch(const std::exception& error) {
            std::fprintf(stderr,"Flight loading failed: %s\n",error.what());
            if(!sceneReady)scene.reset();
            g_Frontend->OpenMain();
            g_Frontend->SetError("Nao foi possivel carregar o voo. Tente novamente.");
        }
        previous_time=glfwGetTime();
        return false;
    };
    if(inspectionRun) {
        SelectFrontendAircraft(*g_Frontend,initialAircraft,1280,720);
        if(!startFlight(initialAircraft)) {
            missionMarker.reset();collisionDebug.reset();hud.reset();frontendScreen.reset();
            scene.reset();g_Frontend.reset();glfwTerminate();return EXIT_FAILURE;
        }
        previous_time=smoke_start=glfwGetTime();
        try {
            if(collision_check) {
                if(!scene->InspectCollisions(MainAircraft,std::string(argv[2])+".txt"))
                    throw std::runtime_error("Collision integration inspection failed");
                g_ShowCollisionVolumes=true;
            }
            if(mission_check&&!scene->InspectMission(MainAircraft,std::string(argv[2])+".txt"))
                throw std::runtime_error("Mission integration inspection failed");
        } catch(const std::exception& error) {
            std::fprintf(stderr,"Inspection failed: %s\n",error.what());
            g_CameraInput.reset();missionMarker.reset();collisionDebug.reset();hud.reset();
            frontendScreen.reset();scene.reset();g_Frontend.reset();glfwTerminate();return EXIT_FAILURE;
        }
        if(smoke_test&&argc>3) {
            const std::string mode=argv[3];
            if(mode=="cockpit")g_Camera.SetMode(CameraType::COCKPIT,g_CameraFrame);
            else if(mode=="free") {
                g_Camera.SetMode(CameraType::FREE,g_CameraFrame);
                CameraMovement movement;movement.vertical=1;
                g_Camera.Update(movement,2.0f,g_CameraFrame);
            }
        }
    }
    previous_time=smoke_start=glfwGetTime();
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);glFrontFace(GL_CCW);

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        frameLimiter.Wait();
        if(menuInspectionActive)frontendInspection.Prepare(window,*g_Frontend);
        if(hud_check)hudInspection.Prepare(window,g_Camera,g_CameraFrame);
        if(missionInspectionActive)missionInspection.Prepare(window);
        // Read the latest mouse events immediately before computing this view.
        glfwPollEvents();
        if(glfwWindowShouldClose(window))break;
        int framebufferWidth,framebufferHeight;
        glfwGetFramebufferSize(window,&framebufferWidth,&framebufferHeight);
        if(framebufferWidth<=0 || framebufferHeight<=0) {
            glfwWaitEventsTimeout(.1);
            previous_time=glfwGetTime();
            continue;
        }
        if(g_Frontend->Active()) {
            const auto action=g_MenuAction;g_MenuAction=frontend::Action::None;
            if(action==frontend::Action::Quit)glfwSetWindowShouldClose(window,GL_TRUE);
            else if(action==frontend::Action::Resume&&sceneReady) {
                // Leaving aircraft selection without starting keeps the current
                // flight and its actual aircraft, including the menu's label.
                for(std::size_t current=0;current<AircraftCatalog().size();++current)
                    if(AircraftCatalog()[current].id==loadedAircraft)
                        SelectFrontendAircraft(*g_Frontend,current,framebufferWidth,framebufferHeight);
                g_Frontend->Close();
            }
            else if(action==frontend::Action::MainMenu)g_Frontend->OpenMain();
            else if(action==frontend::Action::Start)startFlight(g_Frontend->SelectedIndex());
            if(g_Frontend->Active()&&!glfwWindowShouldClose(window)) {
                if(menuInspectionActive)frontendInspection.Draw(window,*frontendScreen,*g_Frontend,glfwGetTime());
                else frontendScreen->Draw(window,*g_Frontend,glfwGetTime());
                if(menuInspectionActive&&frontendInspection.Capture(window)) {
                    if(keep_open) {
                        menuInspectionActive=false;g_Frontend->OpenMain();glfwSetWindowSize(window,1280,720);
                    } else glfwSetWindowShouldClose(window,GL_TRUE);
                }
                glfwSwapBuffers(window);
            }
            previous_time=glfwGetTime();
            continue;
        }
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.60f, 0.76f, 0.90f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        //calculammos o estado do avião
        double current_time = glfwGetTime();
        const double elapsed = std::max(current_time - previous_time,0.0);
        float dt = static_cast<float>(elapsed);
        if(g_ResetFlightRequested) {
            scene->ResetPlayer(MainAircraft);g_ResetFlightRequested=false;g_PlayerCrashed=false;
            flightMission.Reset();flightMission.Update(scene->PlayerMissionSample(MainAircraft),0);
            g_CameraFrame=scene->Cameras(MainAircraft);g_Camera.Reset(g_CameraFrame);
        }
        const auto previousPose=GameScene::PhysicalPose(MainAircraft);
        const double physicsStep=std::min(dt,0.05f);
        // Keep the existing physics step limit; camera travel uses real elapsed time.
        if(!g_PlayerCrashed) {
            MainModel.update(&MainAircraft,static_cast<float>(physicsStep));
            scene->ResolvePlayer(MainAircraft,previousPose,physicsStep);
            g_PlayerCrashed=scene->PlayerContact().state==collision::AircraftState::Crashed;
        }
        flightMission.Update(scene->PlayerMissionSample(MainAircraft),physicsStep);
        previous_time = current_time;
        scene->Update(elapsed); // Advance traffic/lights once; cockpit draw passes only read them.
        g_CameraFrame=scene->Cameras(MainAircraft);
        g_CameraInput->Update(dt);
        const CameraPose camera=texture_check?inspection.Camera(g_CameraFrame,scene->TrafficPose()):g_Camera.Pose(g_CameraFrame);
        const bool cockpit=texture_check?inspection.Cockpit():g_Camera.Mode()==CameraType::COCKPIT;
        glm::mat4 view=Matrix_Camera_View(glm::vec4(camera.position,1),
            glm::vec4(camera.forward,0),glm::vec4(camera.up,0));

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.5f;  // Posição do "near plane"
        float farplane  = -camera.farPlane; // Posição do "far plane"

        if (g_UsePerspectiveProjection || cockpit)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = camera.fieldOfView;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a distancia guardada no controlador de cameras.
            float t = 1.5f*g_Camera.OrbitDistance()/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Keep map depth precision while allowing the cockpit panel very close.
        if(cockpit) {
            scene->Draw(g_GpuProgramID,MainAircraft,view,projection,camera.position,SceneLayer::WORLD);
            glClear(GL_DEPTH_BUFFER_BIT);
            glm::mat4 interior=Matrix_Perspective(camera.fieldOfView,g_ScreenRatio,-camera.nearPlane,-100.0f);
            glUniformMatrix4fv(g_projection_uniform,1,GL_FALSE,glm::value_ptr(interior));
            scene->Draw(g_GpuProgramID,MainAircraft,view,interior,camera.position,SceneLayer::AIRCRAFT);
        } else {
            scene->Draw(g_GpuProgramID,MainAircraft,view,projection,camera.position);
        }

        if(g_ShowCollisionVolumes)collisionDebug->Draw(scene->PlayerProbes(MainAircraft),view,projection,camera.position);
        if(g_ShowMissionMarker)missionMarker->Draw(window,view,projection,camera.position);

        FlightTelemetry readings=CalculateFlightTelemetry(MainAircraft.get_position(),MainAircraft.get_velocity(),
            MainAircraft.get_forward(),MainAircraft.get_up(),MainAircraft.get_throttle());
        if(hud_check)readings=hudInspection.Telemetry(readings);
        hud->Update(readings,elapsed);
        if(g_ShowInfoText) {
            hud->Draw(window,readings);
            int helpWidth,helpHeight;glfwGetWindowSize(window,&helpWidth,&helpHeight);
            if(helpWidth>=650||helpHeight>=360) {
                const char* mode=cockpit?"cockpit":g_Camera.Mode()==CameraType::FREE?"livre":"externa";
                char label[160];
                int logicalWidth,logicalHeight;glfwGetWindowSize(window,&logicalWidth,&logicalHeight);
                // Corner help has independent scales from the instruments and FPS.
                const float headerScale=1.2f,controlsScale=1.05f,speedScale=1.125f;
                const float margin=12.0f;
                const float charPixels=TextRendering_CharWidth(window)*logicalWidth*.5f;
                const auto fits=[&](const std::string& text,float scale,float reserve) {
                    return text.size()*charPixels*scale<=logicalWidth-2*margin-reserve;
                };
                const float firstBaseline=std::max(26.0f,.04f*logicalHeight);
                const float lineStep=std::max(28.0f,TextRendering_LineHeight(window)*logicalHeight*.5f*headerScale);
                const float textX=-1+2*margin/logicalWidth;
                const auto printHelp=[&](const std::string& text,float baseline,float scale) {
                    TextRendering_PrintString(window,text,textX,1-2*baseline/logicalHeight,scale);
                };
                snprintf(label,sizeof(label),"Camera: %s | C: cockpit  F: livre  L: externa | H: HUD",mode);
                if(!fits(label,headerScale,96))snprintf(label,sizeof(label),"%s | C/F/L: cameras",mode);
                if(!fits(label,headerScale,96))snprintf(label,sizeof(label),"%s | C/F/L",mode);
                printHelp(label,firstBaseline,headerScale);
                const char* controls=cockpit?"Botao direito: olhar | Setas/PgUp/PgDn: cabeca | Home: centralizar":
                    g_Camera.Mode()==CameraType::FREE?"WASD: mover | Q/E: descer/subir | Shift: acelerar | Botao direito: olhar":
                    "Botao direito: orbitar | Rodinha: distancia | Home: restaurar";
                if(!fits(controls,controlsScale,0))controls=cockpit?
                    "Dir.: olhar | Setas/Pg: cabeca | Home: reset":g_Camera.Mode()==CameraType::FREE?
                    "WASD: mover | Q/E: altura | Dir.: olhar":
                    "Dir.: orbitar | Roda: distancia | Home: reset";
                if(logicalWidth>=650)printHelp(controls,firstBaseline+lineStep,controlsScale);
                if(g_Camera.Mode()==CameraType::FREE) {
                    snprintf(label,sizeof(label),"Camera livre: %.0f m/s | Rodinha: ajustar",g_Camera.FreeSpeed());
                    if(!fits(label,speedScale,0))snprintf(label,sizeof(label),"Livre: %.0f m/s",g_Camera.FreeSpeed());
                    float instrumentsTop=static_cast<float>(logicalHeight);
                    for(const auto& dial:CalculateHudLayout(logicalWidth,logicalHeight).dials)
                        instrumentsTop=std::min(instrumentsTop,dial.y-dial.radius);
                    const float baseline=firstBaseline+lineStep*(logicalWidth>=650?2:1);
                    if(baseline<instrumentsTop-4)printHelp(label,baseline,speedScale);
                }
            }
        }
        if(g_ShowInfoText||g_PlayerCrashed) {
            char status[180];
            const auto& contact=scene->PlayerContact();
            const char* state=contact.state==collision::AircraftState::Grounded?"No solo":
                              contact.state==collision::AircraftState::Crashed?"Acidente":"Em voo";
            if(g_PlayerCrashed)snprintf(status,sizeof(status),"%s: %s | Enter: reiniciar",state,collision::CrashReasonText(contact.reason));
            else snprintf(status,sizeof(status),"%s | Enter: reiniciar | B: colisoes | M: destino %s",state,
                          g_ShowMissionMarker?"visivel":"oculto");
            int w,h;glfwGetWindowSize(window,&w,&h);
            if(w>=650) {
                const float baseline=std::max(26.0f,.04f*h)+84.0f;
                TextRendering_PrintString(window,status,-1+24.0f/w,1-2*baseline/h,w<650?.85f:1.0f);
            }
        }
        const auto& missionState=flightMission.Snapshot();
        const auto visibleMission=missionInspectionActive?missionInspection.Preview(missionState):missionState;
        if(g_ShowInfoText||visibleMission.stage==mission::Stage::Completed||visibleMission.stage==mission::Stage::Failed)
            missionOverlay.Draw(window,visibleMission);
        TextRendering_ShowFramesPerSecond(window);
        if(texture_check && inspection.Capture(window))glfwSetWindowShouldClose(window,GL_TRUE);
        if(hud_check && hudInspection.Capture(window))glfwSetWindowShouldClose(window,GL_TRUE);
        if(missionInspectionActive&&missionInspection.Capture(window)) {
            if(keep_open) {missionInspectionActive=false;glfwSetWindowSize(window,1280,720);}
            else glfwSetWindowShouldClose(window,GL_TRUE);
        }

        if (smoke_test)
        {
            const GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                fprintf(stderr,"OpenGL error in smoke test: 0x%x\n",error);
                exit_code = EXIT_FAILURE;
                glfwSetWindowShouldClose(window,GL_TRUE);
            }
            if (++rendered_frames == 60) {
                if (argc > 2) {
                    int width,height;
                    glfwGetFramebufferSize(window,&width,&height);
                    std::vector<unsigned char> pixels(size_t(width)*height*3);
                    glPixelStorei(GL_PACK_ALIGNMENT,1);
                    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
                    std::ofstream screenshot(argv[2],std::ios::binary);
                    screenshot << "P6\n" << width << " " << height << "\n255\n";
                    for(int row=height-1;row>=0;--row)
                        screenshot.write(reinterpret_cast<const char*>(pixels.data()+size_t(row)*width*3),width*3);
                    if(!screenshot)exit_code=EXIT_FAILURE;
                }
                const glm::vec3 p=MainAircraft.get_position();
                printf("Smoke test: 60 frames, position (%.3f, %.3f, %.3f), status %d\n",p.x,p.y,p.z,exit_code);
                const double elapsed=glfwGetTime()-smoke_start;
                printf("Frame pacing: %.3f s for 59 intervals, %.1f fps\n",elapsed,59.0/elapsed);
                if(!keep_open)glfwSetWindowShouldClose(window,GL_TRUE);
            }
        }

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

    }

    // Finalizamos o uso dos recursos do sistema operacional
    g_CameraInput.reset();
    g_Frontend.reset();
    frontendScreen.reset();
    collisionDebug.reset();
    missionMarker.reset();
    hud.reset();
    scene.reset();
    glfwTerminate();

    // Fim do programa
    return exit_code;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + Caelum/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       +--+ render/shaders/
    //          |
    //          o-- shader_vertex.glsl
    //          |
    //          o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/render/shaders/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/render/shaders/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice e que pertencem ao mesmo "smoothing group".

    // Obtemos a lista dos smoothing groups que existem no objeto
    std::set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            assert(sgroup >= 0);
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve( 3*num_vertices );

    // Processamos um smoothing group por vez
    for (const unsigned int & sgroup : sgroup_ids)
    {
        std::vector<int> num_triangles_per_vertex(num_vertices, 0);
        std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

        // Acumulamos as normais dos vértices de todos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                glm::vec4  vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                    vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
                }

                const glm::vec4  a = vertices[0];
                const glm::vec4  b = vertices[1];
                const glm::vec4  c = vertices[2];

                const glm::vec4  n = crossproduct(b-a,c-a);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        // Computamos a média das normais acumuladas
        std::vector<size_t> normal_indices(num_vertices, 0);

        for (size_t vertex_index = 0; vertex_index < vertex_normals.size(); ++vertex_index)
        {
            if (num_triangles_per_vertex[vertex_index] == 0)
                continue;

            glm::vec4 n = vertex_normals[vertex_index] / (float)num_triangles_per_vertex[vertex_index];
            n /= norm(n);

            model->attrib.normals.push_back( n.x );
            model->attrib.normals.push_back( n.y );
            model->attrib.normals.push_back( n.z );

            size_t normal_index = (model->attrib.normals.size() / 3) - 1;
            normal_indices[vertex_index] = normal_index;
        }

        // Escrevemos os índices das normais para os vértices dos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[ idx.vertex_index ];
                }
            }
        }

    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        // Em 2026-05-18, corrigido bug encontrado pelo aluno Arthur Prediger:
        // std::numeric_limits<float>::min() retorna o menor valor positivo
        // normalizado representável, não o menor valor possível (negativo). Para
        // inicializar o limite máximo da bounding box com um valor "muito
        // pequeno", deve ser usado std::numeric_limits<float>::lowest()
        const float minval = std::numeric_limits<float>::lowest();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
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
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if(width<=0 || height<=0)return;
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Callbacks only forward events; CameraInput handles capture and control routing.
void MouseButtonCallback(GLFWwindow* window,int button,int action,int mods)
{
    if(g_Frontend&&g_Frontend->Active()) {
        if(button==GLFW_MOUSE_BUTTON_LEFT&&action==GLFW_PRESS) {
            double x,y;int w,h;glfwGetCursorPos(window,&x,&y);glfwGetWindowSize(window,&w,&h);
            const auto next=g_Frontend->Click(x,y,w,h);
            if(next!=frontend::Action::None)g_MenuAction=next;
        }
        return;
    }
    if(g_CameraInput)g_CameraInput->MouseButton(button,action);
}
void CursorPosCallback(GLFWwindow* window,double x,double y)
{
    if(g_Frontend&&g_Frontend->Active()) {
        int w,h;glfwGetWindowSize(window,&w,&h);g_Frontend->Move(x,y,w,h);return;
    }
    if(g_CameraInput)g_CameraInput->Cursor(x,y);
}
void ScrollCallback(GLFWwindow* window,double xoffset,double yoffset)
{
    if(g_Frontend&&g_Frontend->Active())return;
    if(g_CameraInput)g_CameraInput->Scroll(yoffset);
}
void FocusCallback(GLFWwindow* window,int focused)
{
    if(g_CameraInput)g_CameraInput->Focus(focused==GLFW_TRUE);
}

void Correcao_KeyCallback(int key, int action, int mod);

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // =======================
    // Não modifique esta chamada! Ela é utilizada para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    Correcao_KeyCallback(key, action, mod);
    // =======================

    if(g_Frontend&&g_Frontend->Active()) {
        if(action!=GLFW_PRESS&&action!=GLFW_REPEAT)return;
        frontend::Key menuKey;
        bool recognized=true;
        switch(key) {
        case GLFW_KEY_UP:menuKey=frontend::Key::Up;break;
        case GLFW_KEY_DOWN:menuKey=frontend::Key::Down;break;
        case GLFW_KEY_LEFT:menuKey=frontend::Key::Left;break;
        case GLFW_KEY_RIGHT:menuKey=frontend::Key::Right;break;
        case GLFW_KEY_TAB:menuKey=frontend::Key::Tab;break;
        case GLFW_KEY_ENTER:case GLFW_KEY_KP_ENTER:menuKey=frontend::Key::Enter;break;
        case GLFW_KEY_ESCAPE:menuKey=frontend::Key::Escape;break;
        default:recognized=false;break;
        }
        // Navigation may repeat; activation/cancel never repeats while held.
        if(recognized&&(action==GLFW_PRESS||(menuKey!=frontend::Key::Enter&&menuKey!=frontend::Key::Escape))) {
            int w,h;glfwGetWindowSize(window,&w,&h);
            const auto next=g_Frontend->KeyPress(menuKey,w,h,(mod&GLFW_MOD_SHIFT)!=0);
            if(next!=frontend::Action::None)g_MenuAction=next;
        }
        return;
    }
    if(key==GLFW_KEY_ESCAPE&&action==GLFW_PRESS&&g_Frontend) {
        if(g_CameraInput)g_CameraInput->Focus(false);
        g_Frontend->OpenPause();return;
    }
    if(g_CameraInput && g_CameraInput->Key(key,action))return;

    if(key==GLFW_KEY_ENTER&&action==GLFW_PRESS)g_ResetFlightRequested=true;
    if(key==GLFW_KEY_B&&action==GLFW_PRESS)g_ShowCollisionVolumes=!g_ShowCollisionVolumes;
    if(key==GLFW_KEY_M&&action==GLFW_PRESS)g_ShowMissionMarker=!g_ShowMissionMarker;
    if(g_PlayerCrashed&&(key==GLFW_KEY_W||key==GLFW_KEY_S))return;

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    //se o usuário clica w ou s, muda-se o throtlle do avião
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        MainModel.increase_throttle(&MainAircraft);
    }
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        MainModel.decrease_throttle(&MainAircraft);
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :


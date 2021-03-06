#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "game.hpp"
#include "particle.hpp"
#include "emitter.hpp"
#include "camera.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "SOIL/SOIL.h"

#include <stdio.h>
#include <string.h>

double Game::last_update = 0;
bool Game::vsync = false;
float Game::speed = 1;
float Game::gravity = -9.81f;

vec3 Game::wind_direction = vec3(10, 0, 0);
float Game::wind_strength = 0;

char* Game::textures[100];
int Game::texture_count = 0;

GLFWwindow* Game::window = nullptr;
Camera* Game::camera = nullptr;
bool Game::context = false;
bool Game::axis = true;
DemoType Game::demo = DemoType::NONE;
void glfw_error_callback(int error, const char* text) {
    fprintf(stderr, "glfw error: %s\n", text);
}

Game::Game(int argc, char** argv) {
    // Init all the glfw stuff
    glfwSetErrorCallback(glfw_error_callback);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    if (argc < 2) // if we have an option, make it fullscreen
        Game::window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Matt's COMP37111 particle simulator", NULL, NULL);
    else
        Game::window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Matt's COMP37111 particle simulator", glfwGetPrimaryMonitor(), NULL); 

    if (!Game::window) {
        fprintf(stderr, "Failed to create glfw window!\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(Game::window);
    glfwSwapInterval(Game::vsync);
    glfwSetWindowSizeCallback(Game::window, Game::Reshape);
    glfwSetKeyCallback(Game::window, Game::Keyboard);

    Game::Reshape(Game::window, SCREEN_WIDTH, SCREEN_HEIGHT);

    Game::InitImgui();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    Camera* camera = new Camera(window);
    this->camera = camera;

    // Load textures
    Game::LoadTexture("circle_01");
    Game::LoadTexture("magic_01");
    Game::LoadTexture("fire_01");
    Game::LoadTexture("muzzle_01");
    Game::LoadTexture("star_07");
    Game::LoadTexture("twirl_01");

    // init our scene
    Game::InitScene();

    // main loop
    while (!glfwWindowShouldClose(Game::window)) {
        Game::Display();
        glfwSwapBuffers(Game::window);

        glfwPollEvents();
        Game::Update();
    }

    glfwDestroyWindow(Game::window);
    glfwTerminate();

}

void Game::Reshape(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, (GLfloat)w / (GLfloat)h, 1.0, DRAW_DISTANCE);
    glMatrixMode(GL_MODELVIEW);
}

void Game::Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS) {
        Game::camera->context = !Game::context;
        if (Game::camera->context)
            glfwSetInputMode(Game::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(Game::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) exit(0);
}

double Game::last_draw = 0;
double Game::frame_time = 0;

void Game::Display() {
    double time_now = glfwGetTime();
    Game::frame_time = time_now - Game::last_draw;
    Game::last_draw = time_now;

    glClear(GL_COLOR_BUFFER_BIT); // clear the screen
    glLoadIdentity(); // load identity matrix

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Game::camera->Transform();

    if (Game::axis)
        Game::RenderAxis();

    Game::RenderScene();
    Emitter::RenderAll();
    Game::RenderGui();
    
}

void Game::Update() {
    double time_now = glfwGetTime();
    double dt = time_now - Game::last_update;
    dt = dt * Game::speed;
    Game::last_update = time_now;

    Game::camera->Update(dt);
    Emitter::UpdateAll(dt);
    Game::UpdateScene(dt);
}

void Game::InitImgui() {
    glewInit();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(Game::window, true);

    const char* glsl_version = "#version 130";

    ImGui_ImplOpenGL3_Init(glsl_version);
}

vec3 Game::RandVec3() {
    float rx = 1.0f - 2*Game::RandFloat();
    float ry = 1.0f - 2*Game::RandFloat();
    float rz = 1.0f - 2*Game::RandFloat();

    return glm::normalize(vec3(rx, ry, rz));
}

float Game::RandFloat() {
    return ((float)rand() / RAND_MAX);
}

GLint Game::LoadTexture(char* name) {
    char path[25];
    sprintf(path, "textures/%s.png", name);
    GLint tex = SOIL_load_OGL_texture(
        path,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        NULL
    );

    printf("texture: %d\n", tex);

    if (tex == 0) {
        fprintf(stderr, "Failed to load texture %s\n", path);
        return 0;
    } else {
        char* name_ptr = (char*)malloc(1+strlen(name));
        memcpy(name_ptr, name, 1+strlen(name));

        Game::textures[tex] = name_ptr;
        Game::texture_count++;
        return tex;
    }
}

void Game::RenderAxis() {

    glLineWidth(2.0);
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0);       // X axis - red
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(AXIS_SIZE, 0.0, 0.0);
    glColor3f(0.0, 1.0, 0.0);       // Y axis - green
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, AXIS_SIZE, 0.0);
    glColor3f(0.0, 0.0, 1.0);       // Z axis - blue
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, AXIS_SIZE);
    glEnd();

}

void Game::RenderGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Particle Simulator");

    ImGui::Text("camera pos: (%.2f, %.2f, %.2f)", Game::camera->pos.x, Game::camera->pos.y, Game::camera->pos.z);
    ImGui::SameLine();

    if (ImGui::Button("reset"))
        Game::camera->pos = vec3(100, 100, 100);
        
    if (ImGui::Button("Create new emitter")) {
        Emitter* emitter = new Emitter(
            vec3(0,0,0),
            vec3(0, 0, 0), vec3(10, 10, 10),
            4, ParticleType::POINT,
            vec4(0, 0.3, 1, 1), vec4(1, 1, 1, 0), vec4(0.1, 0.1, 0.1, 0),
            0.5, 0.1,
            1000
        );
    }

    ImGui::SliderFloat("Sim Speed", &Game::speed, 0.000f, 5.0f);
    ImGui::SliderFloat("Gravity", &Game::gravity, -50.f, 50.f);

    ImGui::InputFloat3("Wind dir", glm::value_ptr(Game::wind_direction));
    ImGui::SliderFloat("Wind speed", &Game::wind_strength, 0, 100);

    ImGui::Checkbox("Draw axis", &Game::axis);

    if (ImGui::Button("Exit context mode")) {
        Game::camera->context = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }  

    if (ImGui::Button("Clear emitters"))
        Emitter::RemoveAll();


    ImGui::End();


    ImGui::Begin("Examples");
    if (ImGui::Button("Motion accuracy demo")) {
        Game::demo = DemoType::MOTION;
        Game::InitScene();
    }

    ImGui::End();

    Game::RenderFPS();
    Emitter::RenderMenus();
    Game::RenderSceneGui();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool fpsRecording = false;
double fpsStartTime = 0;
unsigned int frames = 0;

void Game::RenderFPS() {
    ImGui::Begin("Performance");
    ImGui::Text("Particles: %d", Particle::count);
    ImGui::Text("FPS: %.3f (Avg: %.3f)", ((float)1.0f / Game::frame_time), ImGui::GetIO().Framerate);
    if (ImGui::Checkbox("vsync", &Game::vsync))
        glfwSwapInterval(Game::vsync);

    if (frames > 0) {
        if (fpsRecording)
            ImGui::Text("Measuring frame rate...");
        else
            ImGui::Text("10 second FPS avg: %.2f fps", ((float)frames / 10.0f));
    }

    if (ImGui::Button("Record 10s FPS avg")) {
        fpsRecording = true;
        fpsStartTime = glfwGetTime();
        frames = 0;
    }

    ImGui::End();
}

Emitter* motionDemoEmitter;
bool demoHitGround = false;
double demoHitGroundTime = 0;

void Game::InitScene() {
    

    if (Game::demo == DemoType::MOTION) {
        Emitter::RemoveAll();
        demoHitGround = false;
        demoHitGroundTime = 0;

        // create a single particle emitter at position (0, 100, 0)
        motionDemoEmitter = new Emitter(
            vec3(0, 100, 0),
            vec3(0, 0, 0), vec3(0, 0, 0),
            10, ParticleType::POINT,
            vec4(0, 0, 1, 1), vec4(1, 0, 0, 1),vec4(0,0,0,0),
            100, 0, 0
        );


    }
    
}



void Game::UpdateScene(double dt) {
    if (fpsRecording) {
        if (glfwGetTime() - fpsStartTime >= 10.0f)
            fpsRecording = false;
        else
            frames++;
    }

    if (Game::demo == DemoType::MOTION && motionDemoEmitter != nullptr) {
        if (motionDemoEmitter->head != nullptr) {
            Particle* particle = motionDemoEmitter->head;
            if (particle->pos.y <= 0 && !demoHitGround) {
                demoHitGroundTime = particle->age;
                demoHitGround = true;
            }
        }
    }
}

void Game::RenderScene() {

}


void Game::RenderSceneGui() {
    if (Game::demo == DemoType::MOTION) {
        ImGui::Begin("Motion demo");
        

        if (motionDemoEmitter->head == nullptr) {
            if (ImGui::Button("Start"))
                motionDemoEmitter->Emit(1);
        } else {
            Particle* particle = motionDemoEmitter->head;
            ImGui::Text("particle age: %.2f", particle->age);
            ImGui::Text("position: (%.2f, %.2f, %.2f)", particle->pos.x, particle->pos.y, particle->pos.z);

            if (demoHitGround)
                ImGui::Text("Hit y=0 at %.2fs", demoHitGroundTime);   
        }

        ImGui::End();

        
    }
}
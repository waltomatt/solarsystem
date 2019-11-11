#ifndef GAME_H
#define GAME_H

#include "GLFW/glfw3.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#define FOV 60
#define DRAW_DISTANCE 500

typedef glm::vec3 vec3;
typedef glm::vec4 vec4;

class Game {
    public:
        static float gravity;
        static vec3 wind_direction;
        static float wind_strength;

        Game(int argc, char** argv);
        //~Game();

        static void Update();
        static void Display();
        static void Reshape(GLFWwindow* window, int w, int h);
        static void Keyboard(unsigned char key, int x, int y);

        static double last_draw;
        static GLFWwindow* window;
        

};

#endif
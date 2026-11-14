
#include "glad.h"
#include "glfw3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr int screenWidth = 800;
constexpr int screenHeight = 600;

constexpr float paddleWidth = 16.0f;
constexpr float paddleHeight = 110.0f;
constexpr float ballSize = 18.0f;

constexpr float paddleSpeed = 420.0f;
constexpr float initialBallSpeed = 330.0f;
constexpr int winningScore = 5;

struct Vertex
{
    float x, y;
    float r, g, b;
};

struct Paddle
{
    float x;
    float y;
};

struct Ball
{
    float x;
    float y;
    float vx;
    float vy;
};

Paddle player1{40.0f, 300.0f};
Paddle player2{744.0f, 300.0f};

Ball ball{400.0f, 300.0f, 0.0f, 0.0f};

int score1 = 0;
int score2 = 0;

bool gameStarted = false;
bool gameOver = false;

unsigned int shaderProgram = 0;
unsigned int VAO = 0;
unsigned int VBO = 0;

const char* vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "out vec3 vertexColor;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(aPos.x / 400.0 - 1.0,"
    " 1.0 - aPos.y / 300.0, 0.0, 1.0);\n"
    "    vertexColor = aColor;\n"
    "}\n";

const char* fragmentShaderSource =
    "#version 330 core\n"
    "in vec3 vertexColor;\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(vertexColor, 1.0);\n"
    "}\n";

// --------------------------------------------------
// WINDOW RESIZE
// --------------------------------------------------

void framebufferSizeCallback(
    GLFWwindow* window, int width, int height)
{
    (void)window;

    if (width <= 0 || height <= 0)
        return;

    float targetAspect =
        static_cast<float>(screenWidth) / screenHeight;

    float windowAspect =
        static_cast<float>(width) / height;

    int viewportWidth = width;
    int viewportHeight = height;

    if (windowAspect > targetAspect)
        viewportWidth = static_cast<int>(height * targetAspect);
    else
        viewportHeight = static_cast<int>(width / targetAspect);

    glViewport(
        (width - viewportWidth) / 2,
        (height - viewportHeight) / 2,
        viewportWidth,
        viewportHeight
    );
}

// --------------------------------------------------
// SHADERS
// --------------------------------------------------

unsigned int createShader(
    unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = GL_FALSE;

    glGetShaderiv(
        shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[512];

        glGetShaderInfoLog(
            shader, 512, nullptr, infoLog);

        std::cerr << "Shader error:\n"
                  << infoLog << std::endl;

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool setupShaders()
{
    unsigned int vertexShader =
        createShader(GL_VERTEX_SHADER, vertexShaderSource);

    unsigned int fragmentShader =
        createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (vertexShader == 0 || fragmentShader == 0)
    {
        if (vertexShader != 0)
            glDeleteShader(vertexShader);

        if (fragmentShader != 0)
            glDeleteShader(fragmentShader);

        return false;
    }

    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int success = GL_FALSE;

    glGetProgramiv(
        shaderProgram, GL_LINK_STATUS, &success);

    if (!success)
    {
        char infoLog[512];

        glGetProgramInfoLog(
            shaderProgram, 512, nullptr, infoLog);

        std::cerr << "Program linking error:\n"
                  << infoLog << std::endl;

        return false;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        0,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, x))
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, r))
    );

    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

// --------------------------------------------------
// DRAWING FUNCTIONS
// --------------------------------------------------

void addRectangle(
    std::vector<Vertex>& vertices,
    float x, float y,
    float width, float height,
    float red, float green, float blue)
{
    Vertex a{x, y, red, green, blue};
    Vertex b{x + width, y, red, green, blue};
    Vertex c{x + width, y + height, red, green, blue};
    Vertex d{x, y + height, red, green, blue};

    vertices.insert(
        vertices.end(),
        {a, b, c, a, c, d}
    );
}

// Simple 5 x 7 pixel font
std::array<int, 7> glyph(char c)
{
    switch (c)
    {
    case 'A': return {14,17,17,31,17,17,17};
    case 'B': return {30,17,17,30,17,17,30};
    case 'C': return {14,17,16,16,16,17,14};
    case 'D': return {30,17,17,17,17,17,30};
    case 'E': return {31,16,16,30,16,16,31};
    case 'F': return {31,16,16,30,16,16,16};
    case 'G': return {14,17,16,23,17,17,15};
    case 'H': return {17,17,17,31,17,17,17};
    case 'I': return {14,4,4,4,4,4,14};
    case 'J': return {7,2,2,2,18,18,12};
    case 'K': return {17,18,20,24,20,18,17};
    case 'L': return {16,16,16,16,16,16,31};
    case 'M': return {17,27,21,21,17,17,17};
    case 'N': return {17,25,25,21,19,19,17};
    case 'O': return {14,17,17,17,17,17,14};
    case 'P': return {30,17,17,30,16,16,16};
    case 'Q': return {14,17,17,17,21,18,13};
    case 'R': return {30,17,17,30,20,18,17};
    case 'S': return {15,16,16,14,1,1,30};
    case 'T': return {31,4,4,4,4,4,4};
    case 'U': return {17,17,17,17,17,17,14};
    case 'V': return {17,17,17,17,17,10,4};
    case 'W': return {17,17,17,21,21,21,10};
    case 'X': return {17,17,10,4,10,17,17};
    case 'Y': return {17,17,10,4,4,4,4};
    case 'Z': return {31,1,2,4,8,16,31};

    case '0': return {14,17,19,21,25,17,14};
    case '1': return {4,12,4,4,4,4,14};
    case '2': return {14,17,1,2,4,8,31};
    case '3': return {30,1,1,14,1,1,30};
    case '4': return {2,6,10,18,31,2,2};
    case '5': return {31,16,16,30,1,1,30};
    case '6': return {14,16,16,30,17,17,14};
    case '7': return {31,1,2,4,8,8,8};
    case '8': return {14,17,17,14,17,17,14};
    case '9': return {14,17,17,15,1,1,14};

    default: return {0,0,0,0,0,0,0};
    }
}

void addText(
    std::vector<Vertex>& vertices,
    const std::string& text,
    float x, float y,
    float scale,
    float r = 1.0f,
    float g = 1.0f,
    float b = 1.0f)
{
    float cursor = x;

    for (char c : text)
    {
        auto pixels = glyph(c);

        for (int row = 0; row < 7; row++)
        {
            for (int col = 0; col < 5; col++)
            {
                if (pixels[row] & (1 << (4 - col)))
                {
                    addRectangle(
                        vertices,
                        cursor + col * scale,
                        y + row * scale,
                        scale, scale,
                        r, g, b
                    );
                }
            }
        }

        cursor += 6 * scale;
    }
}

// --------------------------------------------------
// GAME FUNCTIONS
// --------------------------------------------------

void resetBall(int direction)
{
    ball.x = screenWidth / 2.0f;
    ball.y = screenHeight / 2.0f;

    ball.vx = 0.0f;
    ball.vy = 0.0f;

    gameStarted = false;

    // Store the next serve direction in vx.
    ball.vx = direction * initialBallSpeed;
}

void resetGame()
{
    player1.y = 300.0f;
    player2.y = 300.0f;

    score1 = 0;
    score2 = 0;

    gameOver = false;

    resetBall(1);
}

void startBall()
{
    if (gameStarted || gameOver)
        return;

    gameStarted = true;

    if (ball.vx == 0.0f)
        ball.vx = initialBallSpeed;

    ball.vy = initialBallSpeed * 0.45f;
}

void updateGame(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        player1.y -= paddleSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        player1.y += paddleSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        player2.y -= paddleSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        player2.y += paddleSpeed * deltaTime;

    player1.y = std::clamp(
        player1.y, paddleHeight / 2.0f,
        screenHeight - paddleHeight / 2.0f
    );

    player2.y = std::clamp(
        player2.y, paddleHeight / 2.0f,
        screenHeight - paddleHeight / 2.0f
    );

    if (!gameStarted || gameOver)
        return;

    ball.x += ball.vx * deltaTime;
    ball.y += ball.vy * deltaTime;

    // Top and bottom wall collision
    if (ball.y - ballSize / 2.0f <= 0.0f)
    {
        ball.y = ballSize / 2.0f;
        ball.vy = std::abs(ball.vy);
    }

    if (ball.y + ballSize / 2.0f >= screenHeight)
    {
        ball.y = screenHeight - ballSize / 2.0f;
        ball.vy = -std::abs(ball.vy);
    }

    // Player 1 paddle collision
    if (ball.vx < 0.0f &&
        ball.x - ballSize / 2.0f <= player1.x + paddleWidth &&
        ball.x + ballSize / 2.0f >= player1.x &&
        std::abs(ball.y - player1.y) <=
            (paddleHeight + ballSize) / 2.0f)
    {
        ball.x = player1.x + paddleWidth + ballSize / 2.0f;

        float hitPosition =
            (ball.y - player1.y) / (paddleHeight / 2.0f);

        ball.vx = std::min(std::abs(ball.vx) * 1.06f, 650.0f);
        ball.vy = hitPosition * 420.0f;
    }

    // Player 2 paddle collision
    if (ball.vx > 0.0f &&
        ball.x + ballSize / 2.0f >= player2.x &&
        ball.x - ballSize / 2.0f <= player2.x + paddleWidth &&
        std::abs(ball.y - player2.y) <=
            (paddleHeight + ballSize) / 2.0f)
    {
        ball.x = player2.x - ballSize / 2.0f;

        float hitPosition =
            (ball.y - player2.y) / (paddleHeight / 2.0f);

        ball.vx = -std::min(std::abs(ball.vx) * 1.06f, 650.0f);
        ball.vy = hitPosition * 420.0f;
    }

    // Player 2 scores
    if (ball.x < 0.0f)
    {
        score2++;

        if (score2 >= winningScore)
        {
            gameOver = true;
        }
        else
        {
            resetBall(-1);
        }
    }

    // Player 1 scores
    if (ball.x > screenWidth)
    {
        score1++;

        if (score1 >= winningScore)
        {
            gameOver = true;
        }
        else
        {
            resetBall(1);
        }
    }
}

// --------------------------------------------------
// RENDERING
// --------------------------------------------------

void renderGame()
{
    std::vector<Vertex> vertices;
    vertices.reserve(6000);

    // Background
    addRectangle(
        vertices, 0, 0,
        screenWidth, screenHeight,
        0.025f, 0.035f, 0.10f
    );

    // Court border
    addRectangle(
        vertices, 8, 8,
        screenWidth - 16, 4,
        0.15f, 0.45f, 0.80f
    );

    addRectangle(
        vertices, 8, screenHeight - 12,
        screenWidth - 16, 4,
        0.15f, 0.45f, 0.80f
    );

    // Center dashed line
    for (float y = 15; y < screenHeight; y += 35)
    {
        addRectangle(
            vertices,
            397, y, 6, 20,
            0.35f, 0.45f, 0.65f
        );
    }

    // Player paddles
    addRectangle(
        vertices,
        player1.x,
        player1.y - paddleHeight / 2,
        paddleWidth,
        paddleHeight,
        0.10f, 0.85f, 1.0f
    );

    addRectangle(
        vertices,
        player2.x,
        player2.y - paddleHeight / 2,
        paddleWidth,
        paddleHeight,
        1.0f, 0.35f, 0.30f
    );

    // Ball
    addRectangle(
        vertices,
        ball.x - ballSize / 2,
        ball.y - ballSize / 2,
        ballSize,
        ballSize,
        1.0f, 0.95f, 0.35f
    );

    // Player labels and scores
    addText(
        vertices, "PLAYER 1",
        115, 35, 3.0f,
        0.10f, 0.85f, 1.0f
    );

    addText(
        vertices, "PLAYER 2",
        555, 35, 3.0f,
        1.0f, 0.35f, 0.30f
    );

    addText(
        vertices, std::to_string(score1),
        330, 35, 7.0f
    );

    addText(
        vertices, std::to_string(score2),
        440, 35, 7.0f
    );

    if (!gameStarted && !gameOver)
    {
        addText(
            vertices, "PRESS SPACE TO START",
            250, 500, 3.0f,
            1.0f, 0.95f, 0.35f
        );
    }

    if (gameOver)
    {
        addRectangle(
            vertices, 180, 210,
            440, 170,
            0.08f, 0.10f, 0.20f
        );

        addText(
            vertices, "PLAYER",
            310, 235, 4.0f
        );

        if (score1 > score2)
        {
            addText(
                vertices, "1 WINS",
                320, 280, 5.0f,
                0.10f, 0.85f, 1.0f
            );
        }
        else
        {
            addText(
                vertices, "2 WINS",
                320, 280, 5.0f,
                1.0f, 0.35f, 0.30f
            );
        }

        addText(
            vertices, "PRESS R TO RESTART",
            235, 340, 2.5f
        );
    }

    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertices.size() * sizeof(Vertex)),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        static_cast<GLsizei>(vertices.size())
    );

    glfwSwapBuffers(glfwGetCurrentContext());
}

} // namespace

// --------------------------------------------------
// MAIN FUNCTION
// --------------------------------------------------

int main()
{
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed!" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        screenWidth,
        screenHeight,
        "Pong Arena",
        nullptr,
        nullptr
    );

    if (window == nullptr)
    {
        std::cerr << "Window creation failed!" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(
        window, framebufferSizeCallback
    );

    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "GLAD initialization failed!" << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_FAILURE;
    }

    int width, height;

    glfwGetFramebufferSize(window, &width, &height);
    framebufferSizeCallback(window, width, height);

    if (!setupShaders())
    {
        glDeleteProgram(shaderProgram);
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    resetGame();

    double previousTime = glfwGetTime();

    bool wasRestartPressed = false;
    bool wasSpacePressed = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        double currentTime = glfwGetTime();

        float deltaTime = static_cast<float>(
            std::min(currentTime - previousTime, 0.05)
        );

        previousTime = currentTime;

        // Exit
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // Restart
        bool restartPressed =
            glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

        if (restartPressed && !wasRestartPressed)
        {
            resetGame();
        }

        wasRestartPressed = restartPressed;

        // Start
        bool spacePressed =
            glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

        if (spacePressed && !wasSpacePressed)
        {
            if (!gameOver)
                startBall();
        }

        wasSpacePressed = spacePressed;

        // Update game
        if (!gameOver)
        {
            updateGame(window, deltaTime);
        }

        // Window title
        std::string title =
            "Pong Arena | Player 1: " +
            std::to_string(score1) +
            " | Player 2: " +
            std::to_string(score2);

        if (gameOver)
            title += " | GAME OVER";

        glfwSetWindowTitle(window, title.c_str());

        renderGame();
    }

    // Cleanup
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

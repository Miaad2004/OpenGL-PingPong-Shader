#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr float PLAY_RECT_POS = 0.8f;

constexpr float ROCKET_WIDTH = 0.09f;
constexpr float ROCKET_HEIGHT = 0.6f;

constexpr int MAX_COLLISIONS = 3;
constexpr float MAX_BALL_VELOCITY = 1.3f;

float BALL_X = 0.0f, BALL_Y = 0.0f;
float BALL_SPEED = 0.01f;
float BALL_VX = 0.0f, BALL_VY = 0.0f;
float BALL_TARGET_X = 0.0f, BALL_TARGET_Y = 0.0f;

float ROCKET_X = -0.6f, ROCKET_Y = 0.0f;
float ROCKET_SPEED = 0.05f;
float ROCKET_VELOCITY = 0.0f;

int N_COLLISIONS = 0;
int m = 0;

GLuint prog = 0;
GLint locOffset = -1, locColor = -1;

GLuint vaoRect = 0, vboRect = 0;
GLuint vaoRocket = 0, vboRocket = 0;
GLuint vaoBall = 0, vboBall = 0;
int ballVertexCount = 0;

static char *read_file(const char *fname)
{
    FILE *f = fopen(fname, "rb");
    if (!f)
        return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = new char[sz + 1];
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    return buf;
}

static GLuint load_shader(GLenum type, const char *fname)
{
    GLuint sh = glCreateShader(type);
    char *src = read_file(fname);
    if (!src)
    {
        std::cerr << "Cannot open shader file \"" << fname << "\"\n";
        std::exit(EXIT_FAILURE);
    }
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetShaderInfoLog(sh, 1024, nullptr, log);
        std::cerr << "Shader " << fname << " : " << log << '\n';
        exit(EXIT_FAILURE);
    }
    delete[] src;
    return sh;
}

void make_rectangle(float w, float h, GLuint &vao, GLuint &vbo)
{
    float verts[] = {0, 0, w, 0, w, h, 0, h};
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(0);
}

void make_ball()
{
    constexpr float R = 0.08f;
    const float aspect = static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT;
    const int N = 60;
    ballVertexCount = N + 2;

    std::vector<float> v;
    v.reserve(ballVertexCount * 2);
    v.push_back(0.0f); // center x
    v.push_back(0.0f); // center y
    for (int i = 0; i <= N; i++)
    {
        float t = i * 2.0f * M_PI / N;
        v.push_back((R * std::cos(t)) / aspect);
        v.push_back(R * std::sin(t));
    }

    glGenVertexArrays(1, &vaoBall);
    glBindVertexArray(vaoBall);
    glGenBuffers(1, &vboBall);
    glBindBuffer(GL_ARRAY_BUFFER, vboBall);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(0);
}

void init_gl()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    GLuint vs = load_shader(GL_VERTEX_SHADER, "vertex_shader.glsl");
    GLuint fs = load_shader(GL_FRAGMENT_SHADER, "fragment_shader.glsl");
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Link: " << log << '\n';
        exit(EXIT_FAILURE);
    }
    glUseProgram(prog);
    locOffset = glGetUniformLocation(prog, "uOffset");
    locColor = glGetUniformLocation(prog, "uColor");

    make_rectangle(2.0f * PLAY_RECT_POS, 2.0f * PLAY_RECT_POS, vaoRect, vboRect);
    make_rectangle(ROCKET_WIDTH, ROCKET_HEIGHT, vaoRocket, vboRocket);
    make_ball();

    glBindVertexArray(0);
}

inline void clamp_ball_velocity()
{
    float s = std::sqrt(BALL_VX * BALL_VX + BALL_VY * BALL_VY);
    if (s > MAX_BALL_VELOCITY && s > 0.f)
    {
        BALL_VX = BALL_VX / s * MAX_BALL_VELOCITY;
        BALL_VY = BALL_VY / s * MAX_BALL_VELOCITY;
    }
}

inline void reflect_ball(float nx, float ny)
{
    float dot = BALL_VX * nx + BALL_VY * ny;
    BALL_VX -= 2 * dot * nx;
    BALL_VY -= 2 * dot * ny;
    clamp_ball_velocity();
}

bool rocket_ball_collision()
{
    return BALL_X >= ROCKET_X &&
           BALL_X <= ROCKET_X + ROCKET_WIDTH &&
           BALL_Y >= ROCKET_Y - ROCKET_HEIGHT &&
           BALL_Y <= ROCKET_Y;
}
void handle_ball_collisions()
{
    if (rocket_ball_collision())
    {
        reflect_ball(1, 0);
        m++;
        BALL_SPEED += m * 0.0001f;
    }
    if (BALL_Y >= PLAY_RECT_POS)
    {
        BALL_Y = PLAY_RECT_POS;
        reflect_ball(0, -1);
    }
    if (BALL_Y <= -PLAY_RECT_POS)
    {
        BALL_Y = -PLAY_RECT_POS;
        reflect_ball(0, 1);
    }

    if (BALL_X >= PLAY_RECT_POS)
    {
        BALL_X = PLAY_RECT_POS;
        reflect_ball(-1, 0);
    }
    if (BALL_X <= -PLAY_RECT_POS)
    {
        BALL_X = -PLAY_RECT_POS;
        N_COLLISIONS++;
        if (N_COLLISIONS >= MAX_COLLISIONS)
        {
            std::cout << "Game Over! You caught the ball " << m << " times.\n";
            exit(0);
        }
        reflect_ball(1, 0);
    }
}

void renderBitmapString(float x, float y, void *font, const char *string)
{
    glRasterPos2f(x, y);
    for (const char *c = string; *c != '\0'; c++)
        glutBitmapCharacter(font, *c);
}

void draw_gradient_bg()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glUseProgram(0);

    glBegin(GL_QUADS);
    // top color (light blue)
    glColor3f(0.7f, 0.85f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glVertex2f(1.0f, 1.0f);
    // bottom color (purple)
    glColor3f(0.85f, 0.75f, 1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    draw_gradient_bg();
    glUseProgram(prog);

    // play rectangle
    glBindVertexArray(vaoRect);
    glLineWidth(3.0f); 
    glUniform2f(locOffset, -PLAY_RECT_POS, -PLAY_RECT_POS);
    glUniform3f(locColor, 242 / 255.f, 134 / 255.f, 51 / 255.f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glLineWidth(1.0f); 

    //  rocket
    glBindVertexArray(vaoRocket);
    glUniform2f(locOffset, ROCKET_X, ROCKET_Y - ROCKET_HEIGHT);
    glUniform3f(locColor, 0.8f, 0.2f, 0.95f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // ball
    glBindVertexArray(vaoBall);
    glUniform2f(locOffset, BALL_X, BALL_Y);
    glUniform3f(locColor, 0.2f, 0.6f, 1.0f); 
    glDrawArrays(GL_TRIANGLE_FAN, 0, ballVertexCount);

    // score
    glUseProgram(0);
    glColor3f(0.1f, 0.1f, 0.1f);
    std::string scoreText = "Score: " + std::to_string(m);
    renderBitmapString(-0.95f, 0.9f, GLUT_BITMAP_HELVETICA_18, scoreText.c_str());

    std::string heartsText = "Hearts: " + std::to_string(MAX_COLLISIONS - N_COLLISIONS);
    renderBitmapString(0.7f, 0.9f, GLUT_BITMAP_HELVETICA_18, heartsText.c_str());

    glutSwapBuffers();
}

void update_ball(int)
{
    BALL_X += BALL_VX * BALL_SPEED;
    BALL_Y += BALL_VY * BALL_SPEED;
    handle_ball_collisions();
    glutPostRedisplay();
    glutTimerFunc(16, update_ball, 0);
}

void update_rocket(int)
{
    if (ROCKET_VELOCITY != 0.f)
    {
        ROCKET_Y += ROCKET_VELOCITY;
        if (ROCKET_Y > PLAY_RECT_POS)
            ROCKET_Y = PLAY_RECT_POS;
        if (ROCKET_Y < -PLAY_RECT_POS + ROCKET_HEIGHT)
            ROCKET_Y = -PLAY_RECT_POS + ROCKET_HEIGHT;
        glutPostRedisplay();
    }
    glutTimerFunc(16, update_rocket, 0);
}

void keyboard_down(int key, int, int)
{
    if (key == GLUT_KEY_UP)
        ROCKET_VELOCITY = ROCKET_SPEED;
    else if (key == GLUT_KEY_DOWN)
        ROCKET_VELOCITY = -ROCKET_SPEED;
}
void keyboard_up(int key, int, int)
{
    if (key == GLUT_KEY_UP || key == GLUT_KEY_DOWN)
        ROCKET_VELOCITY = 0.f;
}

void mouse(int btn, int state, int x, int y)
{
    if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        float nx = (2.f * x / WINDOW_WIDTH) - 1.f;
        float ny = -(2.f * y / WINDOW_HEIGHT) + 1.f;
        float dx = nx - BALL_X, dy = ny - BALL_Y;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d != 0.f)
        {
            BALL_VX = dx / d;
            BALL_VY = dy / d;
            clamp_ball_velocity();
        }
        BALL_TARGET_X = nx;
        BALL_TARGET_Y = ny;
        glutTimerFunc(0, update_ball, 0);
    }
}

enum MenuOptions
{
    MENU_EXIT,
    MENU_START_OVER,
    MENU_LEVEL_EASY,
    MENU_LEVEL_MEDIUM,
    MENU_LEVEL_HARD
};

void set_level(int level)
{
    switch (level)
    {
    case MENU_LEVEL_EASY:
        BALL_SPEED = 0.008f;
        break;
    case MENU_LEVEL_MEDIUM:
        BALL_SPEED = 0.015f;
        break;
    case MENU_LEVEL_HARD:
        BALL_SPEED = 0.03f;
        break;
    }
}

void reset_game()
{
    BALL_X = BALL_Y = 0.0f;
    BALL_TARGET_X = BALL_TARGET_Y = 0.0f;
    BALL_VX = BALL_VY = 0.0f;
    ROCKET_Y = 0.0f;
    ROCKET_VELOCITY = 0.0f;
    N_COLLISIONS = 0;
    m = 0;
    glutPostRedisplay();
}

void menu_handler(int option)
{
    switch (option)
    {
    case MENU_EXIT:
        exit(0);
        break;
    case MENU_START_OVER:
        reset_game();
        break;
    case MENU_LEVEL_EASY:
    case MENU_LEVEL_MEDIUM:
    case MENU_LEVEL_HARD:
        set_level(option);
        break;
    }
}

int main(int argc, char **argv)
{
    srand(time(nullptr));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("pingpong shader");
    glewInit();

    init_gl();

    glutDisplayFunc(display);
    glutSpecialFunc(keyboard_down);
    glutSpecialUpFunc(keyboard_up);
    glutMouseFunc(mouse);

    glutTimerFunc(0, update_rocket, 0);

    // Menu setup
    int levelMenu = glutCreateMenu(menu_handler);
    glutAddMenuEntry("Easy", MENU_LEVEL_EASY);
    glutAddMenuEntry("Medium", MENU_LEVEL_MEDIUM);
    glutAddMenuEntry("Hard", MENU_LEVEL_HARD);

    glutCreateMenu(menu_handler);
    glutAddMenuEntry("Exit", MENU_EXIT);
    glutAddMenuEntry("Start Over", MENU_START_OVER);
    glutAddSubMenu("Level", levelMenu);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutMainLoop();
    return 0;
}

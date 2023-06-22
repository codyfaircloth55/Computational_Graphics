#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class


using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
const char* const WINDOW_TITLE = "Cody Faircloth Final Project"; // Macro for window title

// Variables for window width and height
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Stores the GL data relative to a given mesh
struct GLMesh
{
    GLuint vao[9];         // Handle for the vertex array object
    GLuint vbo[9];         // Handle for the vertex buffer object
    GLuint nVertices[9];    // Number of indices of the mesh
};

// Main GLFW window
GLFWwindow* gWindow = nullptr;
// Triangle mesh data
GLMesh gMesh;
// Texture
GLuint deskTextureId, monitorTextureId, pcTextureId, filingCabinetTextureId, speakerTextureId, keyboardTextureId;
glm::vec2 gUVScale(1.0f, 1.0f);
GLint gTexWrapMode = GL_REPEAT;

// Shader programs
GLuint gProgramId;
GLuint gLampProgramId;

// camera
Camera gCamera(glm::vec3(0.0f, 0.0f, 5.0f));
float gLastX = WINDOW_WIDTH / 2.0f;
float gLastY = WINDOW_HEIGHT / 2.0f;
bool gFirstMouse = true;
bool viewProjection = true;
float cameraSpeed = 0.5;

// timing
float gDeltaTime = 0.0f; // time between current frame and last frame
float gLastFrame = 0.0f;

// Light color, position, and scale
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);
glm::vec3 gLightPosition(0.0f, 0.0f, 50.0f);
glm::vec3 gLightScale(0.5f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char*[], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh &mesh);
void UDestroyMesh(GLMesh &mesh);
bool UCreateTexture(const char* filename, GLuint &textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar * vertexShaderSource = GLSL(440,

    layout (location = 0) in vec3 position; // VAP position 0 for vertex position data
    layout (location = 1) in vec3 normal; // VAP position 1 for normals
    layout (location = 2) in vec2 textureCoordinate;

    out vec3 vertexNormal; // For outgoing normals to fragment shader
    out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
    out vec2 vertexTextureCoordinate;

    //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

        vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

        vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        vertexTextureCoordinate = textureCoordinate;
    }
);


/* Fragment Shader Source Code*/
const GLchar * fragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
    in vec3 vertexFragmentPos; // For incoming fragment position
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor; // For outgoing cube color to the GPU

    // Uniform / Global variables for object color, light color, light position, and camera/view position
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    uniform sampler2D uTexture; // Useful when working with multiple textures
    uniform vec2 uvScale;

    void main()
    {
        /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

        //Calculate Ambient lighting*/
        float ambientStrength = 0.2f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        //Calculate Diffuse lighting*/
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
        float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        //Calculate Specular lighting*/
        float specularIntensity = 0.2f; // Set specular light strength
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
        //Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);


/* Lamp Shader Source Code*/
const GLchar * lampVertexShaderSource = GLSL(440,

    layout (location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
    }
);


/* Fragment Shader Source Code*/
const GLchar * lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

    void main()
    {
        fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
    }
);


// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char *image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader programs
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load textures
    const char* fileCabinetTexFilename = "../../resources/textures/filecabinetfront.jpeg";
    const char* pcTexFilename = "../../resources/textures/PCTexture.jpg";
    const char* monitorTexFilename = "../../resources/textures/MonitorTexture.png";
    const char* keyboardTexFilename = "../../resources/textures/KeyboardTexture.png";
    const char* deskTexFilename = "../../resources/textures/DeskTexture.jpg";
    const char* speakerTexFilename = "../../resources/textures/SpeakerTexture.jpg";

    if (!UCreateTexture(deskTexFilename, deskTextureId))
    {
        cout << "Failed to load texture " << deskTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(monitorTexFilename, monitorTextureId))
    {
        cout << "Failed to load texture " << monitorTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(pcTexFilename, pcTextureId))
    {
        cout << "Failed to load texture " << pcTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(fileCabinetTexFilename, filingCabinetTextureId))
    {
        cout << "Failed to load texture " << fileCabinetTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(speakerTexFilename, speakerTextureId))
    {
        cout << "Failed to load texture " << speakerTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(keyboardTexFilename, keyboardTextureId))
    {
        cout << "Failed to load texture " << keyboardTexFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(deskTextureId);
    UDestroyTexture(monitorTextureId);
    UDestroyTexture(pcTextureId);
    UDestroyTexture(filingCabinetTextureId);
    UDestroyTexture(speakerTextureId);
    UDestroyTexture(keyboardTextureId);

    // Release shader programs
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        viewProjection = true;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        viewProjection = false;

    
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset > 0 && cameraSpeed < 0.1f)
        cameraSpeed += 0.01f;
    if (yoffset < 0 && cameraSpeed > 0.01f)
        cameraSpeed -= 0.01f;
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
        case GLFW_MOUSE_BUTTON_LEFT:
        {
            if (action == GLFW_PRESS)
                cout << "Left mouse button pressed" << endl;
            else
                cout << "Left mouse button released" << endl;
        }
        break;

        case GLFW_MOUSE_BUTTON_MIDDLE:
        {
            if (action == GLFW_PRESS)
                cout << "Middle mouse button pressed" << endl;
            else
                cout << "Middle mouse button released" << endl;
        }
        break;

        case GLFW_MOUSE_BUTTON_RIGHT:
        {
            if (action == GLFW_PRESS)
                cout << "Right mouse button pressed" << endl;
            else
                cout << "Right mouse button released" << endl;
        }
        break;

        default:
            cout << "Unhandled mouse button event" << endl;
            break;
    }
}


// Functioned called to render a frame
void URender()
{

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);
    
    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // CUBE: draw cube
    //----------------
    // Set the shader to be used
    glUseProgram(gProgramId);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective or ortho view
    glm::mat4 projection;
    if (viewProjection) {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        float scale = 120;
        projection = glm::ortho((800.0f / scale), -(900.0f / scale), -(600.0f / scale), (600.0f / scale), -2.5f, 6.5f);
    }

#pragma region Filing Cabinet Binding / Generation
    glm::mat4 scale = glm::scale(glm::vec3(1.0f, 1.0f, 0.5f));
    glm::mat4 rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 translation = glm::translate(glm::vec3(0.75f, 0.0f, -0.25f));
    glm::mat4 model = translation * rotation * scale;
    //Retrieves and passed transform matrices to the shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //Reference matrix uniforms from the shader program for the color light color light position and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint keyLightColorLoc = glGetUniformLocation(gProgramId, "keyLightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    //Pass color, light and camera data to the shader programs corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
    //Activates the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[0]);
    //bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, filingCabinetTextureId);
    //Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);
#pragma endregion

#pragma region DeskTop Binding / Generation
    //Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, deskTextureId);
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(gMesh.vao[1]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[1]);
#pragma endregion

#pragma region PC Binding / Generation
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pcTextureId);
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 0.25f));
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.8f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(gMesh.vao[2]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[2]);
#pragma endregion

#pragma region Keyboard Binding / Generation
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, keyboardTextureId);
    scale = glm::scale(glm::vec3(1.25f, 1.0f, 1.0f));
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(gMesh.vao[3]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[3]);
#pragma endregion

#pragma region Monitor Binding / Generation
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, monitorTextureId);
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(gMesh.vao[4]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[4]);
#pragma endregion

#pragma region Speaker Binding / Generation
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, speakerTextureId);
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(-0.75f, 0.0f, 0.40f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(gMesh.vao[5]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[5]);
#pragma endregion

#pragma region DeskLeg Binding / Generation
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, deskTextureId);
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(gMesh.vao[6]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[6]);
#pragma endregion

#pragma region MonitorStand Binding / Generation
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, monitorTextureId);
    scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(gMesh.vao[7]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[7]);
#pragma endregion

#pragma region Light Binding / Generation
    //Draw Lamp
    glUseProgram(gLampProgramId);
    //transform the cube used as a visual cue for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);
    //reference matrix uniforms from the Lamp Shader Program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");
    //pass matrix data to the lamp shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(gMesh.vao[8]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[8]);
#pragma endregion

    //Deactivate vertex array object
    glBindVertexArray(0);


    //swap buffers and poll IO events
    glfwSwapBuffers(gWindow);
    


    

}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh &mesh)
{
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    //vertex Data
    GLfloat filingCabinetVerts[] = {
        //Position              Normal Cords         Texture Cords
        //Front Face
        -0.25f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.25f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
         0.25f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.25f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.25f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.25f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        //Back Face
        -0.25f, -0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.25f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.25f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.25f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.25f, -0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.25f, -0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        //Right Face
         0.25f, -0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f, -0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f, -0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Left Face
        -0.25f, -0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.25f,  0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.25f, -0.5f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.25f, -0.5f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.25f,  0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.25f,  0.5f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Right Face
         0.25f, -0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f, -0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f, -0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Top Face
        -0.25f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.25f,  0.5f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.25f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.25f,  0.5f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        //Bottom Face
        -0.25f, -0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.25f, -0.5f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.25f, -0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.25f, -0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.25f, -0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.25f, -0.5f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,



    };
    GLfloat desktopVerts[] = {
        //Position            Normal Cords         Texture Cords
        //Front Face
        -1.0f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -1.0f,  0.6f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
         1.0f,  0.6f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  0.6f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -1.0f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         1.0f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        //Back Face
        -1.0f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -1.0f,  0.6f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
         1.0f,  0.6f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         1.0f,  0.6f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -1.0f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         1.0f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        //Right Face
         1.0f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         1.0f,  0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         1.0f,  0.6f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         1.0f,  0.6f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         1.0f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         1.0f,  0.6f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        //Left Face
        -1.0f,  0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -1.0f,  0.5f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -1.0f,  0.6f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -1.0f,  0.6f,  1.0f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -1.0f,  0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -1.0f,  0.6f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        //Top Face
        -1.0f,  0.6f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -1.0f,  0.6f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         1.0f,  0.6f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         1.0f,  0.6f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        -1.0f,  0.6f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         1.0f,  0.6f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        //Bottom Face
        -1.0f,  0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -1.0f,  0.5f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         1.0f,  0.5f,  1.0f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         1.0f,  0.5f,  1.0f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        -1.0f,  0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         1.0f,  0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,


    };
    GLfloat pcVerts[] = {
        //Position             Normal Cords         Texture Cords
        //Front Face
        -0.1f,  0.6f,  0.75f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.1f,  1.0f,  0.75f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
         0.1f,  1.0f,  0.75f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.1f,  1.0f,  0.75f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.1f,  0.6f,  0.75f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.1f,  0.6f,  0.75f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        //Back Face
        -0.1f,  0.6f, -0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.1f,  1.0f, -0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.1f,  1.0f, -0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.1f,  1.0f, -0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.1f,  0.6f, -0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.1f,  0.6f, -0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        //Right Face
         0.1f,  0.6f, -0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  1.0f, -0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  1.0f,  0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  1.0f,  0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  0.6f, -0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  0.6f,  0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Left Face
        -0.1f,  0.6f, -0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  1.0f, -0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  1.0f,  0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  1.0f,  0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  0.6f, -0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  0.6f,  0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Top Face
        -0.1f,  1.0f,  0.75f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  1.0f, -0.75f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  1.0f, -0.75f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  1.0f, -0.75f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  1.0f,  0.75f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  1.0f,  0.75f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        //Bottom Face
        -0.1f,  0.6f,  0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  0.6f, -0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  0.6f, -0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  0.6f, -0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.1f,  0.6f,  0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.1f,  0.6f,  0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
    };
    GLfloat keyboardVerts[] = {
        //Position             Normal Cords        Texture Cords
        //Front Face
        -0.15f, 0.60f, 0.95f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.15f, 0.61f, 0.95f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.15f, 0.61f, 0.95f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.15f, 0.61f, 0.95f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.15f, 0.60f, 0.95f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.15f, 0.60f, 0.95f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        //Back Face
        -0.15f, 0.60f, 0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.15f, 0.61f, 0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.15f, 0.61f, 0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.15f, 0.61f, 0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.15f, 0.60f, 0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.15f, 0.60f, 0.75f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        //Right Face
         0.15f, 0.60f, 0.95f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.61f, 0.95f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.61f, 0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.61f, 0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.60f, 0.95f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.60f, 0.75f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Left Face
        -0.15f, 0.60f, 0.95f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.15f, 0.61f, 0.95f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.15f, 0.61f, 0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.15f, 0.61f, 0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.15f, 0.60f, 0.95f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.15f, 0.60f, 0.75f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Top Face
        -0.15f, 0.61f, 0.95f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.15f, 0.61f, 0.75f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.15f, 0.61f, 0.75f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.15f, 0.61f, 0.75f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        -0.15f, 0.61f, 0.95f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.61f, 0.95f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        //Bottom Face
        -0.15f, 0.60f, 0.95f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.15f, 0.60f, 0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.60f, 0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.60f, 0.75f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.15f, 0.60f, 0.95f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.15f, 0.60f, 0.95f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
    };
    GLfloat monitorVerts[] = {
        //Position             Normal Cords         Texture Cords
        //Front Face
        -0.4f, 0.65f,  0.00f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.4f, 1.00f,  0.00f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
         0.4f, 1.00f,  0.00f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.4f, 1.00f,  0.00f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.4f, 0.65f,  0.00f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.4f, 0.65f,  0.00f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        //Back Face
        -0.4f, 0.65f, -0.01f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.4f, 1.00f, -0.01f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.4f, 1.00f, -0.01f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.4f, 1.00f, -0.01f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.4f, 0.65f, -0.01f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.4f, 0.65f, -0.01f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        //Right Face
         0.4f, 0.65f,  0.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 1.00f,  0.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 1.00f, -0.01f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 1.00f, -0.01f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 0.65f,  0.00f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 0.65f, -0.01f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Left Face
        -0.4f, 0.65f,  0.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 1.00f,  0.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 1.00f, -0.01f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 1.00f, -0.01f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 0.65f,  0.00f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 0.65f, -0.01f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Top Face
        -0.4f, 1.00f,  0.00f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 1.00f, -0.01f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 1.00f, -0.01f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 1.00f, -0.01f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 1.00f,  0.00f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 1.00f,  0.00f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        //Bottom Face
        -0.4f, 0.65f,  0.00f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 0.65f, -0.01f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 0.65f, -0.01f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 0.65f, -0.01f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.4f, 0.65f,  0.00f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.4f, 0.65f,  0.00f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,

    };
    GLfloat speakerVerts[] = {
        //Position              Normal Cords         Texture Cords
        //Front Face
        -0.05f, 0.60f,  0.05f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.05f, 0.75f,  0.05f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
         0.05f, 0.75f,  0.05f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f, 
         0.05f, 0.75f,  0.05f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.05f, 0.60f,  0.05f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.05f, 0.60f,  0.05f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        //Back Face
        -0.05f, 0.60f, -0.05f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.05f, 0.75f, -0.05f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.05f, 0.75f, -0.05f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.05f, 0.75f, -0.05f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.05f, 0.60f, -0.05f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.05f, 0.60f, -0.05f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        //Right Face
         0.05f, 0.60f,  0.05f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.75f,  0.05f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.75f, -0.05f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.75f, -0.05f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.60f,  0.05f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.60f, -0.05f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Left Face
        -0.05f, 0.60f,  0.05f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.05f, 0.75f,  0.05f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f, 
        -0.05f, 0.75f, -0.05f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.05f, 0.75f, -0.05f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.05f, 0.60f,  0.05f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.05f, 0.60f, -0.05f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Top Face
        -0.05f, 0.75f,  0.05f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.05f, 0.75f, -0.05f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.75f, -0.05f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.75f, -0.05f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.05f, 0.75f,  0.05f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.75f,  0.05f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        //Bottom Face
        -0.05f, 0.60f,  0.05f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.05f, 0.60f, -0.05f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.60f, -0.05f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.60f, -0.05f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.05f, 0.60f,  0.05f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.05f, 0.60f,  0.05f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,


    };
    GLfloat desklegVerts[] = {
        //Position             Normal Cords         Texture Cords
        //Front Face
        -1.00f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -1.00f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.95f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.95f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f, 
        -1.00f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.95f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        //Back Face
        -1.00f, -0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -1.00f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.95f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.95f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -1.00f, -0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.95f, -0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        //Right Face
        -0.95f, -0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.95f,  0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.95f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.95f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.95f, -0.5f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f
        -0.95f, -0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        //Left Face
        -1.00f, -0.5f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -1.00f,  0.5f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -1.00f,  0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -1.00f,  0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -1.00f, -0.5f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -1.00f, -0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        //Top Face
        -1.00f,  0.5f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -1.00f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        -0.95f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        -0.95f,  0.5f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        -1.00f,  0.5f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.95f,  0.5f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        //Bottom Face
        -1.00f, -0.5f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -1.00f, -0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        -0.95f, -0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        -0.95f, -0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        -1.00f, -0.5f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.95f, -0.5f,  1.0f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,

    };
    GLfloat monitorstandVerts[] = {
        //Position              Normal Cords         Texture Cords
        //Front Face
        -0.01f, 0.60f, -0.01f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        -0.01f, 0.75f, -0.01f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.01f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.01f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 
        -0.01f, 0.60f, -0.01f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.01f, 0.60f, -0.01f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        //Back Face
        -0.01f, 0.60f, -0.02f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.01f, 0.75f, -0.02f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.02f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.02f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        -0.01f, 0.60f, -0.02f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.01f, 0.60f, -0.02f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        //Right Face
         0.01f, 0.60f, -0.01f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.01f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.02f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.02f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.60f, -0.01f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.60f, -0.02f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Left Face
        -0.01f, 0.60f, -0.01f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.75f, -0.01f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.75f, -0.02f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.75f, -0.02f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.60f, -0.01f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.60f, -0.02f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        //Top Face
        -0.01f, 0.75f, -0.01f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.75f, -0.02f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.02f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.02f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.75f, -0.01f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.75f, -0.01f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
         //Bottom Face
        -0.01f, 0.60f, -0.01f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.60f, -0.02f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.60f, -0.02f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.60f, -0.02f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.01f, 0.60f, -0.01f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
         0.01f, 0.60f, -0.01f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,

    };
    mesh.nVertices[0] = sizeof(filingCabinetVerts) / (sizeof(filingCabinetVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[1] = sizeof(desktopVerts) / (sizeof(desktopVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[2] = sizeof(pcVerts) / (sizeof(pcVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[3] = sizeof(keyboardVerts) / (sizeof(keyboardVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[4] = sizeof(monitorVerts) / (sizeof(monitorVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[5] = sizeof(speakerVerts) / (sizeof(speakerVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[6] = sizeof(desklegVerts) / (sizeof(desklegVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));
    mesh.nVertices[7] = sizeof(monitorstandVerts) / (sizeof(monitorstandVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

#pragma region Filing Cabinet Mesh
    glGenVertexArrays(1, &mesh.vao[0]);
    glGenBuffers(1, &mesh.vbo[0]);
    glBindVertexArray(mesh.vao[0]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(filingCabinetVerts), filingCabinetVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Desk Top Mesh
    glGenVertexArrays(1, &mesh.vao[1]);
    glGenBuffers(1, &mesh.vbo[1]);
    glBindVertexArray(mesh.vao[1]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(desktopVerts), desktopVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region PC Mesh
    glGenVertexArrays(1, &mesh.vao[2]);
    glGenBuffers(1, &mesh.vbo[2]);
    glBindVertexArray(mesh.vao[2]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pcVerts), pcVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Keyboard Mesh
    glGenVertexArrays(1, &mesh.vao[3]);
    glGenBuffers(1, &mesh.vbo[3]);
    glBindVertexArray(mesh.vao[3]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(keyboardVerts), keyboardVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Monitor Mesh
    glGenVertexArrays(1, &mesh.vao[4]);
    glGenBuffers(1, &mesh.vbo[4]);
    glBindVertexArray(mesh.vao[4]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(monitorVerts), monitorVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Speaker Mesh
    glGenVertexArrays(1, &mesh.vao[5]);
    glGenBuffers(1, &mesh.vbo[5]);
    glBindVertexArray(mesh.vao[5]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(speakerVerts), speakerVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Desk Leg Mesh
    glGenVertexArrays(1, &mesh.vao[6]);
    glGenBuffers(1, &mesh.vbo[6]);
    glBindVertexArray(mesh.vao[6]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(desklegVerts), desklegVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Monitor Stand Mesh
    glGenVertexArrays(1, &mesh.vao[7]);
    glGenBuffers(1, &mesh.vbo[7]);
    glBindVertexArray(mesh.vao[7]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(monitorstandVerts), monitorstandVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(10, mesh.vao);
    glDeleteBuffers(10, mesh.vbo);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

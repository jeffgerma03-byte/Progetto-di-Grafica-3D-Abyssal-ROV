#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Camera.h"

// --- 1. SHADERS DI BASE ---
// Vertex Shader: Riceve i vertici spaziali e applica le matrici Model-View-Projection (MVP)
// (Programmi che girano sulla Scheda Video per disegnare il triangolo)
const char* vertexShaderSource = R"glsl(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

// Fragment Shader: Assegna un colore costante (arancione) ai frammenti generati
const char* fragmentShaderSource = R"glsl(
    #version 410 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 0.5, 0.2, 1.0); 
    }
)glsl";

int main() {
    // --- SETUP CONTESTO E FINESTRA ---
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Abyssal ROV - Tappa 02 (Telecamera)", sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);

    // Catturiamo il mouse dentro la finestra e lo nascondiamo 
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        std::cerr << "Errore GLAD" << std::endl; return -1;
    }

    // Abilitiamo il Depth Test (Z-Buffer) per capire cosa sta davanti e cosa sta dietro 
    glEnable(GL_DEPTH_TEST);

    // --- GEOMETRIA: TRIANGOLO DI RIFERIMENTO ---
    // Definisco i vertici 3D del triangolo di test
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  //sinistra
         0.5f, -0.5f, 0.0f,  //destra
         0.0f,  0.5f, 0.0f   //alto
    };

    // Creazione Vertex Array Object (VAO) e Vertex Buffer Object (VBO)
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Specifico il layout degli attributi (Coordinate di posizione)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // --- COMPILAZIONE SHADER PROGRAM ---
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);


    // --- VARIABILI DI STATO E TEMPO ---
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f)); // Partiamo un po' indietro (Z = 3) rispetto al centro
    sf::Clock deltaClock;
    
    // Variabili per la gestione del mouse
    sf::Vector2i centerMousePosition = sf::Vector2i(window.getSize().x / 2u, window.getSize().y / 2u);
    sf::Mouse::setPosition(centerMousePosition, window);
    bool firstMouse = true;

    bool mouseLocked = true;
    // ======================
    // GAME LOOP PRINCIPALE
    // ======================
    while (window.isOpen()) {

        // Calcolo del DeltaTime per rendere il movimento indipendente dal framerate
        float deltaTime = deltaClock.restart().asSeconds();

        // --- GESTIONE DEGLI EVENTI ---
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) window.close();
            

                if (keyPressed->code == sf::Keyboard::Key::Tab) {
                mouseLocked = !mouseLocked;
                window.setMouseCursorVisible(!mouseLocked);
                window.setMouseCursorGrabbed(mouseLocked);
                }
            }
            // Gestione ridimensionamento dinamico: Aggiornamento Viewport e centro mouse
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                glViewport(0, 0, resized->size.x, resized->size.y);
                centerMousePosition = sf::Vector2i(resized->size.x / 2u, resized->size.y / 2u);
            }
        }

        // --- INPUT: TASTIERA ---
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) camera.ProcessKeyboard(0, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) camera.ProcessKeyboard(1, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) camera.ProcessKeyboard(2, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) camera.ProcessKeyboard(3, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) camera.ProcessKeyboard(4, deltaTime); 
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) camera.ProcessKeyboard(5, deltaTime); 

        // --- INPUT: MOUSE ---
        if (mouseLocked && window.hasFocus()) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (firstMouse) { centerMousePosition = mousePos; firstMouse = false; }
            float xoffset = mousePos.x - centerMousePosition.x;
            float yoffset = centerMousePosition.y - mousePos.y; // Invertito: su schermo Y cresce verso il basso
            camera.ProcessMouseMovement(xoffset, yoffset);
            // Riporta il mouse al centro per non farlo mai uscire dalla finestra
            sf::Mouse::setPosition(centerMousePosition, window);
        }

        // --- FASE DI RENDERING ---
        glClearColor(0.02f, 0.05f, 0.2f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Preparazione e invio delle matrici MVP (Model, View, Projection) allo Shader
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window.getSize().x / (float)window.getSize().y, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f); // Triangolo posizionato all'origine

        // Invio le matrici alla scheda video
        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Disegno il Vertex Array Object (disegno fisico del triangolo)
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.display();
    }

    return 0;
}
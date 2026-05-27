#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "Camera.h"


// --- SHADER TAPPA 03: RENDERING PROCEDURALE ---

// Vertex Shader: Estrae l'altezza (Y) dal vertice elaborato e la passa al Fragment Shader per colorarla
const char* vertexShaderSource = R"glsl(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    
    out float Height; 
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        Height = aPos.y; 
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

// Fragment Shader: Colora proceduralmente la mesh in base all'altezza (Y).
// Più il valore è basso (profondo), più il colore tende al blu scuro; più è alto, più diventa azzurro chiaro.
const char* fragmentShaderSource = R"glsl(
    #version 410 core
    in float Height;
    out vec4 FragColor;
    
    void main() {
        float h = clamp(Height / 20.0, 0.0, 1.0); // Mappa l'altezza tra 0.0 e 1.0
        vec3 deepColor = vec3(0.05, 0.1, 0.2); // Fossa 
        vec3 peakColor = vec3(0.5, 0.5, 0.55); // Picco 
        
        vec3 finalColor = mix(deepColor, peakColor, h);
        FragColor = vec4(finalColor, 1.0);
    }
)glsl";

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Abyssal ROV - Tappa 03 (Terrain)", sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        std::cerr << "Errore GLAD" << std::endl; return -1;
    }

    glEnable(GL_DEPTH_TEST);

    
    // --- GENERAZIONE DEL TERRAIN (HEIGHTMAP)---

    // 1. Lettura dell'immagine in scala di grigi
    sf::Image heightmap;
    if (!heightmap.loadFromFile("../Cartella-risorse/heightmap.png")) {
        std::cerr << "Errore: impossibile caricare heightmap.png! Controlla il nome e la cartella." << std::endl;
        return -1;
    }

    unsigned int width = heightmap.getSize().x;
    unsigned int height = heightmap.getSize().y;
    std::cout << "Mappa caricata: " << width << "x" << height << " pixel." << std::endl;

    // Vettori dinamici per immagazzinare i dati della geometria procedurale
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Parametri di calibrazione del terreno
    float scaleXZ = 0.5f;   // Distanza orizzontale tra i vertici (espande la mappa)
    float scaleY = 25.0f;   // Altezza massima delle "montagne" (espande i valori di altezza)

    // 2. Generazione dei Vertici (X, Y, Z)
    for (unsigned int z = 0; z < height; ++z) {
        for (unsigned int x = 0; x < width; ++x) {
            sf::Color color = heightmap.getPixel(sf::Vector2u(x, z));
            float normalizedHeight = color.r / 255.0f; // Leggiamo il grigio (da 0.0 a 1.0)

            // Centriamo la mappa sottraendo la metà  della dimensione e applichiamo la scala
            float vx = (x - (float)width / 2.0f) * scaleXZ;
            float vy = normalizedHeight * scaleY;
            float vz = (z - (float)height / 2.0f) * scaleXZ;

            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
        }
    }

    // 3. Generazione degli Indici (EBO)
    // Unisce i punti della griglia (vertici) per formare 2 triangoli per ogni "quadrato" (cella)
    for (unsigned int z = 0; z < height - 1; ++z) {
        for (unsigned int x = 0; x < width - 1; ++x) {
            // Calcola l'indice 1D corrispondente alle coordinate 2D
            unsigned int topLeft = z * width + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * width + x;
            unsigned int bottomRight = bottomLeft + 1;

            // Primo triangolo della cella (ordine Antiorario)
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Secondo triangolo della cella (ordine Antiorario)
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // 4. Trasferimento dati alla memoria GPU (VBO, VAO, EBO)
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    // Carica l'array di vertici nel VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Carica l'array degli indici nell'Element Buffer Object (EBO)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Definisce la struttura dei dati: 3 float (X,Y,Z) per ogni vertice
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Compilazione Shader
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

    // Posizione di partenza comoda per vedere la mappa (30 metri di altezza)
    Camera camera(glm::vec3(0.0f, 30.0f, 0.0f)); 
    sf::Clock deltaClock;
    
    sf::Vector2i centerMousePosition = sf::Vector2i(window.getSize().x / 2u, window.getSize().y / 2u);
    sf::Mouse::setPosition(centerMousePosition, window);
    bool firstMouse = true;

    bool mouseLocked = true;

    
    // --- GAME LOOP ---
    
    while (window.isOpen()) {
        float deltaTime = deltaClock.restart().asSeconds();

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

            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                glViewport(0, 0, resized->size.x, resized->size.y);
                centerMousePosition = sf::Vector2i(resized->size.x / 2u, resized->size.y / 2u);
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) camera.ProcessKeyboard(0, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) camera.ProcessKeyboard(1, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) camera.ProcessKeyboard(2, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) camera.ProcessKeyboard(3, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) camera.ProcessKeyboard(4, deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)) camera.ProcessKeyboard(5, deltaTime);

        if (mouseLocked && window.hasFocus()) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (firstMouse) { centerMousePosition = mousePos; firstMouse = false; }
            float xoffset = mousePos.x - centerMousePosition.x;
            float yoffset = centerMousePosition.y - mousePos.y; 
            camera.ProcessMouseMovement(xoffset, yoffset);
            sf::Mouse::setPosition(centerMousePosition, window);
        }

        glClearColor(0.01f, 0.03f, 0.1f, 1.0f); // Un blu molto scuro per lo sfondo
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)window.getSize().x / (float)window.getSize().y, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Rendering tramite Indici: uso glDrawElements al posto di glDrawArrays
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        window.display();
    }

    return 0;
}
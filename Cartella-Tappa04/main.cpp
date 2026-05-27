#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "Camera.h"


// --- SHADER TAPPA 04: TEXTURE MAPPING E BLENDING ---

// Vertex Shader: Riceve X,Y,Z (location = 0) e le nuove coordinate U,V (location = 1)
const char* vertexShaderSource = R"glsl(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord; // Coordinate delle Texture
    
    out vec2 TexCoord; 
    out float Height;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        Height = aPos.y; 
        TexCoord = aTexCoord;     // Passiamo le coordinate al Fragment Shader
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

// Fragment Shader: Campiona la texture della sabbia e applica il gradiente di profondità
const char* fragmentShaderSource = R"glsl(
    #version 410 core
    out vec4 FragColor;
    
    in vec2 TexCoord;
    in float Height;
    
    uniform sampler2D texture1; // L'immagine sand.png
    
    void main() {
        // Estraiamo il colore del pixel dalla fotografia originale
        vec4 texColor = texture(texture1, TexCoord);
        
        // Calcoliamo quanto siamo in profondità (0.0 = fondo, 1.0 = cima)
        float depthFactor = clamp(Height / 15.0, 0.2, 1.0); 
        vec3 deepBlue = vec3(0.1, 0.2, 0.4); 
        
        // Scuriamo la sabbia mischiandola col blu in base alla profondità
        vec3 finalColor = mix(deepBlue * texColor.rgb, texColor.rgb, depthFactor);
        
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

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Abyssal ROV - Tappa 04 (Texture)", sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        std::cerr << "Errore GLAD" << std::endl; return -1;
    }
    glEnable(GL_DEPTH_TEST);

    sf::Image heightmap;
    if (!heightmap.loadFromFile("../Cartella-risorse/heightmap.png")) {
        std::cerr << "Errore caricamento heightmap.png!" << std::endl; return -1;
    }
    unsigned int width = heightmap.getSize().x;
    unsigned int height = heightmap.getSize().y;

    
    // --- 2. GENERAZIONE VERTICI CON COORDINATE UV ---
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float scaleXZ = 0.5f;   
    float scaleY = 25.0f;  
    
    // Moltiplicatore UV (Tiling): Ripete l'immagine per mantenere alta la densità di pixel al metro quadro
    float textureTiling = 100.0f; 

    for (unsigned int z = 0; z < height; ++z) {
        for (unsigned int x = 0; x < width; ++x) {
            sf::Color color = heightmap.getPixel(sf::Vector2u(x, z));
            float normalizedHeight = color.r / 255.0f;

            // X, Y, Z (Coordinate nello spazio)
            vertices.push_back((x - (float)width / 2.0f) * scaleXZ);
            vertices.push_back(normalizedHeight * scaleY);
            vertices.push_back((z - (float)height / 2.0f) * scaleXZ);

            // U, V (Coordinate Texture proiettate lungo gli assi X e Z per incollare le texture della sabbia)
            vertices.push_back(((float)x / width) * textureTiling);
            vertices.push_back(((float)z / height) * textureTiling);
        }
    }

    for (unsigned int z = 0; z < height - 1; ++z) {
        for (unsigned int x = 0; x < width - 1; ++x) {
            unsigned int topLeft = z * width + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * width + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft); indices.push_back(bottomLeft); indices.push_back(topRight);
            indices.push_back(topRight); indices.push_back(bottomLeft); indices.push_back(bottomRight);
        }
    }

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    
    // --- AGGIORNAMENTO LAYOUT VERTICI (STRIDE = 5) ---

    // Attributo 0: Posizione (X, Y, Z). Legge i primi 3 float su un blocco totale di 5.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Attributo 1: Coordinate UV (U, V). Legge 2 float applicando un offset iniziale di 3 float (salta la posizione).
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    
    // --- CARICAMENTO E CONFIGURAZIONE TEXTURE SABBIA ---
    
    sf::Image sandTex;
    if (!sandTex.loadFromFile("../Cartella-risorse/sand.png")) {
        std::cerr << "Errore caricamento sand.png!" << std::endl; return -1;
    }
    // Inversione Y: SFML legge le immagini dall'alto verso il basso, OpenGL dal basso verso l'alto: ho dovuto capovolgere l'immagine
    sandTex.flipVertically(); 

    unsigned int texture1;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    // Wrapping: Istruisce OpenGL a ripetere la texture (Tile) quando le UV eccedono 1.0 (ripete l'immagine se usciamo dai bordi)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Mipmapping: Previene l'effetto aliasing in lontananza scalando dinamicamente la texture (rende la sabbia morbida da vicino e sfumata da lontano)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Trasferimento dei dati grezzi del pixel array (dati dell'immagine) dalla CPU alla GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sandTex.getSize().x, sandTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, sandTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

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

    Camera camera(glm::vec3(0.0f, 30.0f, 0.0f)); 
    sf::Clock deltaClock;
    
    sf::Vector2i centerMousePosition = sf::Vector2i(window.getSize().x / 2u, window.getSize().y / 2u);
    sf::Mouse::setPosition(centerMousePosition, window);
    bool firstMouse = true;
    bool mouseLocked = true;


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

        
        glClearColor(0.01f, 0.03f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Assicura l'attivazione della Texture Unit 0 prima del rendering (lego la texture PRIMA di disegnare)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

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

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        window.display();
    }

    return 0;
}
#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "Camera.h"


// --- SHADER TAPPA 05: ILLUMINAZIONE DINAMICA ---

const char* vertexShaderSource = R"glsl(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    
    out vec2 TexCoord; 
    out vec3 FragPos; 
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        TexCoord = aTexCoord; 
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

// Fragment Shader
const char* fragmentShaderSource = R"glsl(
    #version 410 core
    out vec4 FragColor;
    
    in vec2 TexCoord;
    in vec3 FragPos;
    
    uniform sampler2D texture1; 
    
    // Uniforms per la gestione dello Spotlight
    uniform vec3 lightPos;           // Posizione assoluta del faro 
    uniform vec3 lightDir;           // Vettore direzionale (verso dove punta il faro) 
    uniform float lightCutOff;       // Coseno dell'angolo interno (luce massima) 
    uniform float lightOuterCutOff;  // Coseno dell'angolo esterno (luce minima)
    
    void main() {

        // Campionamento del colore base della texture (Albedo)
        vec4 texColor = texture(texture1, TexCoord);
        
        // --- 1. CALCOLO DIREZIONE E CONO (SPOTLIGHT) ---
        // Vettore normalizzato che punta dal frammento verso la luce
        vec3 lightToFrag = normalize(FragPos - lightPos);

        // Prodotto scalare per calcolare il coseno dell'angolo tra
        // la direzione della luce e la direzione del frammento.
        // Nota: inverto lightDir perché punta DALLA luce, mentre lightToFrag punta ALLA luce.
        float theta = dot(lightToFrag, normalize(lightDir));
        
        // Interpolazione lineare per sfumare i bordi del faro (Soft Edges)
        float epsilon = lightCutOff - lightOuterCutOff;
        float intensity = clamp((theta - lightOuterCutOff) / epsilon, 0.0, 1.0);
        
        // --- 2. CALCOLO DELL'ATTENUAZIONE (Distanza) ---
        float distance = length(FragPos - lightPos);
        float constant = 1.0;
        float linear = 0.045;
        float quadratic = 0.003;

        // Formula standard dell'attenuazione: I = 1 / (Kc + Kl*d + Kq*d^2)
        float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
        
        // --- 3. ILLUMINAZIONE FINALE ---
        // Luce Ambientale: Una base debolissima per simulare la visibilità minima naturale anche al di fuori del cono di luce
        vec3 ambient = vec3(0.02, 0.03, 0.08) * texColor.rgb;
    
        // Luce Diffusa: Colore della luce fusa con la texture, moltiplicata per l'intensità del cono e l'attenuazione
        vec3 spotlightColor = vec3(1.5, 1.4, 1.2); 
        vec3 diffuse = spotlightColor * texColor.rgb * intensity * attenuation;
        
        // Addizione tra base ambientale e luce dinamica
        vec3 finalColor = ambient + diffuse;
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

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Abyssal ROV - Tappa 05 (Spotlights)", sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        std::cerr << "Errore GLAD" << std::endl; return -1;
    }
    glEnable(GL_DEPTH_TEST);

    sf::Image heightmap;
    if (!heightmap.loadFromFile("../Cartella-risorse/heightmap.png")) {
        std::cerr << "Errore heightmap!" << std::endl; return -1;
    }
    unsigned int width = heightmap.getSize().x;
    unsigned int height = heightmap.getSize().y;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float scaleXZ = 0.5f;   
    float scaleY = 25.0f;   
    float textureTiling = 100.0f; 

    for (unsigned int z = 0; z < height; ++z) {
        for (unsigned int x = 0; x < width; ++x) {
            sf::Color color = heightmap.getPixel(sf::Vector2u(x, z));
            float normalizedHeight = color.r / 255.0f;

            vertices.push_back((x - (float)width / 2.0f) * scaleXZ);
            vertices.push_back(normalizedHeight * scaleY);
            vertices.push_back((z - (float)height / 2.0f) * scaleXZ);

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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    sf::Image sandTex;
    if (!sandTex.loadFromFile("../Cartella-risorse/sand.png")) {
        std::cerr << "Errore sand.png!" << std::endl; return -1;
    }
    sandTex.flipVertically(); 

    unsigned int texture1;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sandTex.getSize().x, sandTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, sandTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

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

    Camera camera(glm::vec3(0.0f, 15.0f, 0.0f)); 
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

        // Il colore di Clear deve essere nero totale in linea con l'assenza di luce.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        
        // --- INVIO DEI PARAMETRI DEL FARO (UNIFORMS) ---
        
        // Il faro è collegato visivamente alla posizione della telecamera (il sottomarino)
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(camera.Position));
        // Il faro punta verso la direzione in cui stiamo guardando
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, glm::value_ptr(camera.Front));
        
        // Calcolo degli angoli del cono (Inner 12°, Outer 22°) passati come coseno
        glUniform1f(glGetUniformLocation(shaderProgram, "lightCutOff"), glm::cos(glm::radians(12.0f)));
        glUniform1f(glGetUniformLocation(shaderProgram, "lightOuterCutOff"), glm::cos(glm::radians(22.0f)));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        window.display();
    }

    return 0;
}
#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib> 

#include "Camera.h"
#include "ModelLoader.h"


// --- STRUTTURE DATI TAPPA 08: BOIDS ALGORITHM ---

// Definisco l'entità base per il sistema multi-agente.
// Traccia lo stato cinetico indipendente per l'integrazione di Eulero.
struct Boid {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
};

const char* vertexShaderSource = R"glsl(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    layout (location = 2) in vec3 aNormal; 
    
    out vec2 TexCoord; 
    out vec3 FragPos; 
    out vec3 Normal; 
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        TexCoord = aTexCoord; 
        
        Normal = mat3(transpose(inverse(model))) * aNormal; 
        
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 410 core
    out vec4 FragColor;
    
    in vec2 TexCoord;
    in vec3 FragPos;
    in vec3 Normal; 
    
    uniform sampler2D texture1;    
    uniform sampler2D emissiveMap; 
    
    uniform vec3 lightPos;         
    uniform vec3 lightDir;         
    uniform float lightCutOff;     
    uniform float lightOuterCutOff;
    uniform vec3 viewPos; 
    
    uniform bool useNormals; 
    
    void main() {
        vec4 texColor = texture(texture1, TexCoord);
        if (texColor.a < 0.1) discard;
        vec3 emissiveColor = texture(emissiveMap, TexCoord).rgb; 
        
        vec3 lightToFrag = normalize(lightPos - FragPos); 
        float theta = dot(-lightToFrag, normalize(lightDir));
        float epsilon = lightCutOff - lightOuterCutOff;
        float intensity = clamp((theta - lightOuterCutOff) / epsilon, 0.0, 1.0);
        
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.003 * (distance * distance));
        
        float diff = 1.0;
        if (useNormals) {
            vec3 norm = normalize(Normal);
            diff = max(dot(norm, lightToFrag), 0.0);
        }
        
        vec3 ambient = vec3(0.02, 0.03, 0.08) * texColor.rgb;
        if (useNormals) {
            ambient = vec3(0.08, 0.10, 0.15) * texColor.rgb; 
        }
        
        vec3 spotlightColor = vec3(1.5, 1.4, 1.2); 
        vec3 diffuse = spotlightColor * texColor.rgb * diff * intensity * attenuation;
        
        vec3 objectColor = ambient + diffuse + (emissiveColor * 2.0); 
        
        float distToCamera = length(viewPos - FragPos);
        float fogDensity = 0.028; 
        float fogFactor = exp(-pow(distToCamera * fogDensity, 2.0));
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        
        vec3 fogColor = vec3(0.01, 0.02, 0.05); 
        vec3 finalColor = mix(fogColor, objectColor, fogFactor);
        
        FragColor = vec4(finalColor, 1.0);
    }
)glsl";

float getTerrainHeight(float worldX, float worldZ, const sf::Image& hmap, float scaleXZ, float scaleY) {
    int width = hmap.getSize().x;
    int height = hmap.getSize().y;

    int imgX = (int)(worldX / scaleXZ + width / 2.0f);
    int imgZ = (int)(worldZ / scaleXZ + height / 2.0f);

    if (imgX < 0 || imgX >= width || imgZ < 0 || imgZ >= height) {
        return 0.0f; 
    }

    sf::Color color = hmap.getPixel(sf::Vector2u(imgX, imgZ));
    float normalizedHeight = color.r / 255.0f;
    return normalizedHeight * scaleY;
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4;
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Abyssal ROV - Tappa 08 (Boids)", sf::Style::Default, sf::State::Windowed, settings);
    
    // Abilito il V-Sync hardware. Previene il tearing e blocca il rendering alla
    // frequenza del monitor (es. 60Hz), ottimizzando le richieste di frame all'iGPU
    window.setVerticalSyncEnabled(true);
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

    // --- CARICAMENTO RISORSE CONTAINER ---
    sf::Image containerTex;
    if (!containerTex.loadFromFile("../Cartella-risorse/DefaultMaterial_Base_Color.png")) {
        std::cerr << "Errore texture container!" << std::endl; return -1;
    }
    containerTex.flipVertically();

    unsigned int textureContainer;
    glGenTextures(1, &textureContainer);
    glBindTexture(GL_TEXTURE_2D, textureContainer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, containerTex.getSize().x, containerTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, containerTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    sf::Image emissiveImg;
    if (!emissiveImg.loadFromFile("../Cartella-risorse/DefaultMaterial_Emissive.png")) {
        std::cerr << "Errore texture emissiva!" << std::endl; return -1;
    }
    emissiveImg.flipVertically();

    unsigned int textureEmissive;
    glGenTextures(1, &textureEmissive);
    glBindTexture(GL_TEXTURE_2D, textureEmissive);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, emissiveImg.getSize().x, emissiveImg.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, emissiveImg.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    std::vector<float> containerVertices;
    if (!ModelLoader::loadOBJ("../Cartella-risorse/container.obj", containerVertices)) {
        std::cerr << "Errore caricamento container.obj" << std::endl; return -1;
    }

    unsigned int containerVAO, containerVBO;
    glGenVertexArrays(1, &containerVAO);
    glGenBuffers(1, &containerVBO);

    glBindVertexArray(containerVAO);
    glBindBuffer(GL_ARRAY_BUFFER, containerVBO);
    glBufferData(GL_ARRAY_BUFFER, containerVertices.size() * sizeof(float), containerVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // --- CARICAMENTO RISORSE ROCCIA GRANDE ---
    sf::Image largeRockTex;
    if (!largeRockTex.loadFromFile("../Cartella-risorse/large_rock_base.jpeg")) { 
        std::cerr << "Errore texture large_rock_base.jpeg!" << std::endl; return -1;
    }
    largeRockTex.flipVertically();

    unsigned int textureLargeRock;
    glGenTextures(1, &textureLargeRock);
    glBindTexture(GL_TEXTURE_2D, textureLargeRock);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, largeRockTex.getSize().x, largeRockTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, largeRockTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    std::vector<float> largeRockVertices;
    if (!ModelLoader::loadOBJ("../Cartella-risorse/large_rock.obj", largeRockVertices)) { 
        std::cerr << "Errore caricamento large_rock.obj" << std::endl; return -1;
    }

    unsigned int largeRockVAO, largeRockVBO;
    glGenVertexArrays(1, &largeRockVAO);
    glGenBuffers(1, &largeRockVBO);
    glBindVertexArray(largeRockVAO);
    glBindBuffer(GL_ARRAY_BUFFER, largeRockVBO);
    glBufferData(GL_ARRAY_BUFFER, largeRockVertices.size() * sizeof(float), largeRockVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // --- CARICAMENTO RISORSE ROCCIA ALTA ---
    sf::Image tallRockTex;
    if (!tallRockTex.loadFromFile("../Cartella-risorse/tall_rock_basecolor.png")) { 
        std::cerr << "Errore texture tall_rock_basecolor.png!" << std::endl; return -1;
    }
    tallRockTex.flipVertically();

    unsigned int textureTallRock;
    glGenTextures(1, &textureTallRock);
    glBindTexture(GL_TEXTURE_2D, textureTallRock);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tallRockTex.getSize().x, tallRockTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tallRockTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    std::vector<float> tallRockVertices;
    if (!ModelLoader::loadOBJ("../Cartella-risorse/tall_rock.obj", tallRockVertices)) { 
        std::cerr << "Errore caricamento tall_rock.obj" << std::endl; return -1;
    }

    unsigned int tallRockVAO, tallRockVBO;
    glGenVertexArrays(1, &tallRockVAO);
    glGenBuffers(1, &tallRockVBO);
    glBindVertexArray(tallRockVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tallRockVBO);
    glBufferData(GL_ARRAY_BUFFER, tallRockVertices.size() * sizeof(float), tallRockVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // --- CARICAMENTO RISORSE CORALLI ---
    sf::Image coralTex;
    if (!coralTex.loadFromFile("../Cartella-risorse/coralli_texture.png")) {
        std::cerr << "Errore texture coralli_texture.png!" << std::endl; return -1;
    }

    unsigned int textureCoral;
    glGenTextures(1, &textureCoral);
    glBindTexture(GL_TEXTURE_2D, textureCoral);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, coralTex.getSize().x, coralTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, coralTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    std::vector<float> coralVertices;
    if (!ModelLoader::loadOBJ("../Cartella-risorse/coralli.obj", coralVertices)) {
        std::cerr << "Errore caricamento coralli.obj" << std::endl; return -1;
    }

    unsigned int coralVAO, coralVBO;
    glGenVertexArrays(1, &coralVAO);
    glGenBuffers(1, &coralVBO);
    glBindVertexArray(coralVAO);
    glBindBuffer(GL_ARRAY_BUFFER, coralVBO);
    glBufferData(GL_ARRAY_BUFFER, coralVertices.size() * sizeof(float), coralVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // --- CARICAMENTO RISORSE ALGHE ---
    sf::Image algaeTex;
    if (!algaeTex.loadFromFile("../Cartella-risorse/alghe_texture.png")) {
        std::cerr << "Errore texture alghe_texture.png!" << std::endl; return -1;
    }

    unsigned int textureAlgae;
    glGenTextures(1, &textureAlgae);
    glBindTexture(GL_TEXTURE_2D, textureAlgae);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, algaeTex.getSize().x, algaeTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, algaeTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    std::vector<float> algaeVertices;
    if (!ModelLoader::loadOBJ("../Cartella-risorse/alghe.obj", algaeVertices)) {
        std::cerr << "Errore caricamento alghe.obj" << std::endl; return -1;
    }

    unsigned int algaeVAO, algaeVBO;
    glGenVertexArrays(1, &algaeVAO);
    glGenBuffers(1, &algaeVBO);
    glBindVertexArray(algaeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, algaeVBO);
    glBufferData(GL_ARRAY_BUFFER, algaeVertices.size() * sizeof(float), algaeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // --- CARICAMENTO RISORSE PESCE (Tappa 08) ---
    sf::Image fishTex;
    if (!fishTex.loadFromFile("../Cartella-risorse/pesce_texture.png")) {
        std::cerr << "Errore texture pesce_texture.png!" << std::endl; return -1;
    }
    fishTex.flipVertically();

    unsigned int textureFish;
    glGenTextures(1, &textureFish);
    glBindTexture(GL_TEXTURE_2D, textureFish);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fishTex.getSize().x, fishTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, fishTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    std::vector<float> fishVertices;
    if (!ModelLoader::loadOBJ("../Cartella-risorse/pesce.obj", fishVertices)) {
        std::cerr << "Errore caricamento pesce.obj" << std::endl; return -1;
    }

    unsigned int fishVAO, fishVBO;
    glGenVertexArrays(1, &fishVAO);
    glGenBuffers(1, &fishVBO);
    glBindVertexArray(fishVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fishVBO);
    glBufferData(GL_ARRAY_BUFFER, fishVertices.size() * sizeof(float), fishVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // --- CARICAMENTO RISORSE SOTTOMARINO ---
    sf::Image subTex;
    if (!subTex.loadFromFile("../Cartella-risorse/texture_sottomarino.png")) { 
        std::cerr << "Errore texture sottomarino!" << std::endl; return -1;
    }
    subTex.flipVertically();

    unsigned int textureSub;
    glGenTextures(1, &textureSub);
    glBindTexture(GL_TEXTURE_2D, textureSub);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, subTex.getSize().x, subTex.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, subTex.getPixelsPtr());
    glGenerateMipmap(GL_TEXTURE_2D);

    std::vector<float> subVertices;
    if (!ModelLoader::loadOBJ("../Cartella-risorse/sottomarino.obj", subVertices)) { 
        std::cerr << "Errore caricamento sottomarino.obj" << std::endl; return -1;
    }

    unsigned int subVAO, subVBO;
    glGenVertexArrays(1, &subVAO);
    glGenBuffers(1, &subVBO);

    glBindVertexArray(subVAO);
    glBindBuffer(GL_ARRAY_BUFFER, subVBO);
    glBufferData(GL_ARRAY_BUFFER, subVertices.size() * sizeof(float), subVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);


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

    Camera camera(glm::vec3(0.0f, 15.0f, 0.0f)); 
    sf::Clock deltaClock;
    
    sf::Vector2i centerMousePosition = sf::Vector2i(window.getSize().x / 2u, window.getSize().y / 2u);
    sf::Mouse::setPosition(centerMousePosition, window);
    bool firstMouse = true;

    // Inizializzazione Map Limits e Container
    std::vector<glm::vec3> containerPositions = {
        glm::vec3(   0.0f, 0.0f,  -15.0f), 
        glm::vec3( 105.0f, 0.0f,  -95.0f), 
        glm::vec3(-115.0f, 0.0f,  -80.0f), 
        glm::vec3(  85.0f, 0.0f,  110.0f), 
        glm::vec3( -90.0f, 0.0f,   95.0f)  
    };

    // --- ALGORITMO PROCEDURALE DI DISTRIBUZIONE ---
    std::vector<glm::vec3> largeRockPositions;
    std::vector<glm::vec3> tallRockPositions;
    std::vector<glm::vec3> coralPositions;
    std::vector<glm::vec3> algaePositions;
    
    std::srand(42); 
    float mapLimit = 115.0f;
    float safeRadiusFromContainers = 8.0f;
    
    // 1. Generazione Rocce Grandi
    while (largeRockPositions.size() < 70) {
        float rx = ((std::rand() % 2000) / 2000.0f) * (2.0f * mapLimit) - mapLimit;
        float rz = ((std::rand() % 2000) / 2000.0f) * (2.0f * mapLimit) - mapLimit;
        
        bool positionValid = true;
        for (const auto& cPos : containerPositions) {
            if (glm::distance(glm::vec2(rx, rz), glm::vec2(cPos.x, cPos.z)) < safeRadiusFromContainers) {
                positionValid = false;
                break;
            }
        }
        if (positionValid) {
            largeRockPositions.push_back(glm::vec3(rx, 0.0f, rz));
        }
    }

    // 2. Generazione Rocce Alte
    while (tallRockPositions.size() < 70) {
        float rx = ((std::rand() % 2000) / 2000.0f) * (2.0f * mapLimit) - mapLimit;
        float rz = ((std::rand() % 2000) / 2000.0f) * (2.0f * mapLimit) - mapLimit;
        
        bool positionValid = true;
        for (const auto& cPos : containerPositions) {
            if (glm::distance(glm::vec2(rx, rz), glm::vec2(cPos.x, cPos.z)) < safeRadiusFromContainers) {
                positionValid = false;
                break;
            }
        }
        for (const auto& lrPos : largeRockPositions) {
            if (glm::distance(glm::vec2(rx, rz), glm::vec2(lrPos.x, lrPos.z)) < 5.0f) {
                positionValid = false;
                break;
            }
        }
        if (positionValid) {
            tallRockPositions.push_back(glm::vec3(rx, 0.0f, rz));
        }
    }

    // 3. Generazione Coralli
    while (coralPositions.size() < 170) {
        float rx = ((std::rand() % 2000) / 2000.0f) * (2.0f * mapLimit) - mapLimit;
        float rz = ((std::rand() % 2000) / 2000.0f) * (2.0f * mapLimit) - mapLimit;
        
        bool positionValid = true;
        for (const auto& cPos : containerPositions) {
            if (glm::distance(glm::vec2(rx, rz), glm::vec2(cPos.x, cPos.z)) < 5.0f) { positionValid = false; break; }
        }
        if (positionValid) {
            for (const auto& lrPos : largeRockPositions) {
                if (glm::distance(glm::vec2(rx, rz), glm::vec2(lrPos.x, lrPos.z)) < 6.0f) { positionValid = false; break; }
            }
        }
        if (positionValid) {
            for (const auto& trPos : tallRockPositions) {
                if (glm::distance(glm::vec2(rx, rz), glm::vec2(trPos.x, trPos.z)) < 4.0f) { positionValid = false; break; }
            }
        }
        if (positionValid) {
            coralPositions.push_back(glm::vec3(rx, 0.0f, rz));
        }
    }

    // 4. Generazione Alghe (a cluster stretti)
    int numberOfAlgaeClusters = 60;
    int algaePerCluster = 15; 
    
    for (int i = 0; i < numberOfAlgaeClusters; ++i) {
        float cx = ((std::rand() % 2000) / 2000.0f) * (2.0f * mapLimit) - mapLimit;
        float cz = ((std::rand() % 2000) / 2000.0f) * (2.0f * mapLimit) - mapLimit;
        
        bool centerValid = true;
        for (const auto& cPos : containerPositions) {
            if (glm::distance(glm::vec2(cx, cz), glm::vec2(cPos.x, cPos.z)) < safeRadiusFromContainers) { 
                centerValid = false; break; 
            }
        }
        
        if (centerValid) {
            for (int j = 0; j < algaePerCluster; ++j) {
                float offsetX = ((std::rand() % 200) / 200.0f) * 4.0f - 2.0f; 
                float offsetZ = ((std::rand() % 200) / 200.0f) * 4.0f - 2.0f;
                algaePositions.push_back(glm::vec3(cx + offsetX, 0.0f, cz + offsetZ));
            }
        }
    }

    // --- INIZIALIZZAZIONE DATI PER BOIDS ---
    int numBoids = 150;
    std::vector<Boid> boids(numBoids);
    for (auto& b : boids) {
        // Distribuzione iniziale semi-casuale
        b.position = glm::vec3(
            ((std::rand() % 200) / 100.0f - 1.0f) * 40.0f,
            12.0f + ((std::rand() % 100) / 100.0f * 5.0f), // Nuotano a mezz'acqua
            ((std::rand() % 200) / 100.0f - 1.0f) * 40.0f
        );
        b.velocity = glm::vec3(
            ((std::rand() % 200) / 100.0f - 1.0f),
            ((std::rand() % 200) / 100.0f - 1.0f) * 0.2f, // Movimento Y molto morbido
            ((std::rand() % 200) / 100.0f - 1.0f)
        );
        b.velocity = glm::normalize(b.velocity) * 2.0f;
        b.acceleration = glm::vec3(0.0f);
    }

    // Parametri operativi del Flocking
    float perceptionRadius = 6.0f;
    float maxSpeed = 3.5f;
    float maxForce = 2.5f;

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

      
        // ALGORITMO BOIDS - LOGICA DI UPDATE CPU
        for (auto& boid : boids) {
            glm::vec3 separation(0.0f);
            glm::vec3 alignment(0.0f);
            glm::vec3 cohesion(0.0f);
            int total = 0;

            // 1. Calcolo Vettori per Separazione, Allineamento, Coesione
            for (const auto& other : boids) {
                float d = glm::distance(boid.position, other.position);
                if (d > 0.001f && d < perceptionRadius) {
                    glm::vec3 diff = boid.position - other.position;
                    separation += glm::normalize(diff) / d; // Pesi maggiori per pesci vicinissimi
                    alignment += other.velocity;
                    cohesion += other.position;
                    total++;
                }
            }

            if (total > 0) {
                separation /= (float)total;
                alignment /= (float)total;
                cohesion /= (float)total;

                alignment = glm::normalize(alignment) * maxSpeed - boid.velocity;
                cohesion = glm::normalize(cohesion - boid.position) * maxSpeed - boid.velocity;
                
                // Funzione Lambda per troncare la magnitudo della forza ed evitare schizzi fuori mappa
                auto limitForce = [&](glm::vec3& v) { if(glm::length(v) > maxForce) v = glm::normalize(v) * maxForce; };
                limitForce(separation); limitForce(alignment); limitForce(cohesion);
            }

            // 2. Modulazione dei Pesi di Reynolds
            // Favorisco nettamente la repulsione locale rispetto all'aggregazione per un effetto realistico
            boid.acceleration += (separation * 3.5f) + (alignment * 1.0f) + (cohesion * 0.7f);

            // 3. Logica di Fuga Dinamica dal Cono del ROV (Scappano solo se il sottomarino è a meno di 12 metri e li punta)
            float distToSub = glm::distance(boid.position, camera.Position);
            if (distToSub < 12.0f) { 
                glm::vec3 dirToBoid = glm::normalize(boid.position - camera.Position);
                // Verifica di Inquadratura tramite Dot Product tra vettore direzione del boid e fronte telecamera
                float angleDot = glm::dot(dirToBoid, camera.Front);
                // Cutoff di circa 20 gradi
                if (angleDot > glm::cos(glm::radians(20.0f))) { 
                    glm::vec3 fleeForce = glm::normalize(boid.position - camera.Position) * maxSpeed;
                    fleeForce -= boid.velocity;
                    boid.acceleration += fleeForce * 2.5f; // Scatto in avanti se vengono illuminati dal cono di luce
                }
            }

            // 4. Barriere Geografiche Volumetriche (Ground & Sky Bounds)
            float terrainY = getTerrainHeight(boid.position.x, boid.position.z, heightmap, scaleXZ, scaleY);
            if (boid.position.y < terrainY + 3.0f) boid.acceleration.y += 3.0f; // Evita di sbattere sulla sabbia
            if (boid.position.y > 30.0f) boid.acceleration.y -= 1.0f; // Evita di nuotare troppo in superficie

            // Barriere Assi X e Z (Rimbalzo invisibile soft-limit, evitano i confini della mappa)
            if (boid.position.x < -mapLimit + 10.0f) boid.acceleration.x += 1.0f;
            if (boid.position.x > mapLimit - 10.0f) boid.acceleration.x -= 1.0f;
            if (boid.position.z < -mapLimit + 10.0f) boid.acceleration.z += 1.0f;
            if (boid.position.z > mapLimit - 10.0f) boid.acceleration.z -= 1.0f;
        }

        // 5. Integrazione di Eulero per aggiornare posizione e velocità dei boids, con trimming della velocità massima
        for (auto& boid : boids) {
            boid.velocity += boid.acceleration * deltaTime;
            if(glm::length(boid.velocity) > maxSpeed) boid.velocity = glm::normalize(boid.velocity) * maxSpeed; // Trimming Velocity Assoluto
            boid.position += boid.velocity * deltaTime;
            boid.acceleration = glm::vec3(0.0f); // Reset Accumulatore delle Forze per il frame n+1
        }

        
        // SISTEMA DI COLLISIONI FISICHE E RACCOLTA SONDE
        if (camera.Position.x > mapLimit)  camera.Position.x = mapLimit;
        if (camera.Position.x < -mapLimit) camera.Position.x = -mapLimit;
        if (camera.Position.z > mapLimit)  camera.Position.z = mapLimit;
        if (camera.Position.z < -mapLimit) camera.Position.z = -mapLimit;

        float currentTerrainHeight = getTerrainHeight(camera.Position.x, camera.Position.z, heightmap, scaleXZ, scaleY);
        float groundCushion = 1.0f; 
        
        if (camera.Position.y < currentTerrainHeight + groundCushion) {
            camera.Position.y = currentTerrainHeight + groundCushion;
        }

        // --- LOGICA DI RACCOLTA (Collisione a Sfera) ---
        float collectionRadius = 2.0f; 
        for (auto it = containerPositions.begin(); it != containerPositions.end(); ) {
            float containerY = getTerrainHeight(it->x, it->z, heightmap, scaleXZ, scaleY) - 0.4f;
            glm::vec3 realPos = glm::vec3(it->x, containerY, it->z);
            
            float distance = glm::distance(camera.Position, realPos);

            if (distance < collectionRadius) {
                std::cout << "[SISTEMA] Sonda recuperata! Sonde rimanenti: " << (containerPositions.size() - 1) << std::endl;
                it = containerPositions.erase(it);
            } else {
                ++it;
            }
        }

        glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
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

        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(camera.Position));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, glm::value_ptr(camera.Front));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera.Position));
        
        glUniform1f(glGetUniformLocation(shaderProgram, "lightCutOff"), glm::cos(glm::radians(12.0f)));
        glUniform1f(glGetUniformLocation(shaderProgram, "lightOuterCutOff"), glm::cos(glm::radians(22.0f)));

        glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "emissiveMap"), 1);

        // --- 1. RENDERING FONDALE DI SABBIA ---
        glUniform1i(glGetUniformLocation(shaderProgram, "useNormals"), 0); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0); 
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // --- 2. RENDERING DEI CONTAINER ---
        glUniform1i(glGetUniformLocation(shaderProgram, "useNormals"), 1); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureContainer);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureEmissive); 
        
        glBindVertexArray(containerVAO);

        for (size_t i = 0; i < containerPositions.size(); i++) {
            float terrainY = getTerrainHeight(containerPositions[i].x, containerPositions[i].z, heightmap, scaleXZ, scaleY);
            containerPositions[i].y = terrainY - 0.4f; 

            glm::mat4 modelContainer = glm::mat4(1.0f);
            modelContainer = glm::translate(modelContainer, containerPositions[i]);
            modelContainer = glm::scale(modelContainer, glm::vec3(1.0f, 1.0f, 1.0f)); 
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelContainer));
            glDrawArrays(GL_TRIANGLES, 0, containerVertices.size() / 8);
        }

        // --- 2B. RENDERING DELLE ROCCE GRANDI ---
        glUniform1i(glGetUniformLocation(shaderProgram, "useNormals"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureLargeRock);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0); 
        
        glBindVertexArray(largeRockVAO);
        for (size_t i = 0; i < largeRockPositions.size(); i++) {
            float terrainY = getTerrainHeight(largeRockPositions[i].x, largeRockPositions[i].z, heightmap, scaleXZ, scaleY);
            largeRockPositions[i].y = terrainY - 0.15f; 

            glm::mat4 modelRock = glm::mat4(1.0f);
            modelRock = glm::translate(modelRock, largeRockPositions[i]);
            modelRock = glm::scale(modelRock, glm::vec3(1.0f, 1.0f, 1.0f)); 
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelRock));
            glDrawArrays(GL_TRIANGLES, 0, largeRockVertices.size() / 8);
        }

        // --- 2C. RENDERING DELLE ROCCE ALTE ---
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureTallRock);
        
        glBindVertexArray(tallRockVAO);
        for (size_t i = 0; i < tallRockPositions.size(); i++) {
            float terrainY = getTerrainHeight(tallRockPositions[i].x, tallRockPositions[i].z, heightmap, scaleXZ, scaleY);
            tallRockPositions[i].y = terrainY - 0.8f;

            glm::mat4 modelRock = glm::mat4(1.0f);
            modelRock = glm::translate(modelRock, tallRockPositions[i]);
            modelRock = glm::scale(modelRock, glm::vec3(1.0f, 1.0f, 1.0f)); 
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelRock));
            glDrawArrays(GL_TRIANGLES, 0, tallRockVertices.size() / 8);
        }

        // --- 2D. RENDERING DEI CORALLI ---
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureCoral);
        
        glBindVertexArray(coralVAO);
        for (size_t i = 0; i < coralPositions.size(); i++) {
            float terrainY = getTerrainHeight(coralPositions[i].x, coralPositions[i].z, heightmap, scaleXZ, scaleY);
            coralPositions[i].y = terrainY - 0.5f; 

            glm::mat4 modelCoral = glm::mat4(1.0f);
            modelCoral = glm::translate(modelCoral, coralPositions[i]);
            modelCoral = glm::rotate(modelCoral, glm::radians(i * 73.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            modelCoral = glm::rotate(modelCoral, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            
            modelCoral = glm::scale(modelCoral, glm::vec3(30.0f, 30.0f, 30.0f)); 
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCoral));
            glDrawArrays(GL_TRIANGLES, 0, coralVertices.size() / 8);
        }

        // --- 2E. RENDERING DELLE ALGHE ---
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAlgae);
        
        glBindVertexArray(algaeVAO);
        for (size_t i = 0; i < algaePositions.size(); i++) {
            float terrainY = getTerrainHeight(algaePositions[i].x, algaePositions[i].z, heightmap, scaleXZ, scaleY);
            algaePositions[i].y = terrainY - 0.2f; 

            glm::mat4 modelAlgae = glm::mat4(1.0f);
            modelAlgae = glm::translate(modelAlgae, algaePositions[i]);
            
            modelAlgae = glm::rotate(modelAlgae, glm::radians(i * 47.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            
            modelAlgae = glm::scale(modelAlgae, glm::vec3(1.0f, 1.0f, 1.0f)); 
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelAlgae));
            glDrawArrays(GL_TRIANGLES, 0, algaeVertices.size() / 8);
        }

        // --- 2F. RENDERING DEI PESCI (BIOLUMINESCENTI) ---
        glUniform1i(glGetUniformLocation(shaderProgram, "useNormals"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureFish);

        // Utilizza la texture base anche come mappa emissiva per eludere il Phong Shading puro
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureFish); 
        
        glBindVertexArray(fishVAO);
        for (const auto& boid : boids) {
            glm::mat4 modelFish = glm::mat4(1.0f);
            modelFish = glm::translate(modelFish, boid.position);
            
            // Calcolo della rotazione matematica per allineare lo sguardo e farli guardare verso il vettore velocità (LookAt inversa vettoriale)
            if (glm::length(boid.velocity) > 0.001f) {
                glm::vec3 front = glm::normalize(boid.velocity);
                glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), front));
                glm::vec3 up = glm::cross(front, right);
                glm::mat4 rotMatrix(1.0f);
                rotMatrix[0] = glm::vec4(right, 0.0f);
                rotMatrix[1] = glm::vec4(up, 0.0f);
                rotMatrix[2] = glm::vec4(-front, 0.0f); 
                modelFish *= rotMatrix;
            }

            // CORREZIONE ASSE DEL MODELLO IMPORTATO (Pivot errato della Mesh originale scaricata)
            modelFish = glm::rotate(modelFish, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            modelFish = glm::scale(modelFish, glm::vec3(0.15f)); 
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelFish));
            glDrawArrays(GL_TRIANGLES, 0, fishVertices.size() / 8);
        }

        // --- 3. RENDERING DEL SOTTOMARINO ---
        glClear(GL_DEPTH_BUFFER_BIT); 
        
        glUniform1i(glGetUniformLocation(shaderProgram, "useNormals"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureSub);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0); 
        
        glBindVertexArray(subVAO);

        glm::mat4 modelSub = glm::mat4(1.0f);
        
        glm::vec3 zaxis = -camera.Front;
        glm::vec3 xaxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), zaxis));
        glm::vec3 yaxis = glm::cross(zaxis, xaxis);

        glm::mat4 camRot(1.0f);
        camRot[0] = glm::vec4(xaxis, 0.0f);
        camRot[1] = glm::vec4(yaxis, 0.0f);
        camRot[2] = glm::vec4(-zaxis, 0.0f); 

        modelSub = glm::translate(modelSub, camera.Position);
        modelSub = modelSub * camRot;
        
        modelSub = glm::translate(modelSub, glm::vec3(0.0f, 1.0f, -0.2f));
        modelSub = glm::scale(modelSub, glm::vec3(1.0f, 1.0f, 1.0f)); 
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSub));
        glDrawArrays(GL_TRIANGLES, 0, subVertices.size() / 8);

        window.display();
    }

    return 0;
}
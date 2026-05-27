#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>

int main() {
    // 1. Configurazione del contesto OpenGL 4.1 Core, obbliga a usare la pipeline programmabile (Shader, VBO, VAO)
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 4; 
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Core;

    // 2. Creazione della finestra SFML 3.0
    // // Inizializzo la finestra passando risoluzione, titolo, stile di default e le impostazioni OpenGL appena create.
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Abyssal ROV - Tappa 01", sf::Style::Default, sf::State::Windowed, settings);
    
    // Imposta un limite hardware di 60 FPS per evitare un utilizzo eccessivo della CPU/GPU e stabilizzare il Game Loop
    window.setFramerateLimit(60);

    // 3. Inizializzazione di glad
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction))) {
        std::cerr << "Errore: impossibile inizializzare glad!" << std::endl;
        return -1;
    }
    std::cout << "OpenGL Inizializzato con successo. Versione: " << glGetString(GL_VERSION) << std::endl;

    // 4. Colore di pulizia dello schermo (blu scuro)
    glClearColor(0.02f, 0.05f, 0.2f, 1.0f);

    // ==========================================
    // GAME LOOP PRINCIPALE
    // ==========================================
    while (window.isOpen()) {
        //evento di  chiusura della finestra
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        // Pulisce il Color Buffer (applicando il colore di background) e il Depth Buffer (resettando la profondità)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        window.display();
    }

    return 0;
}
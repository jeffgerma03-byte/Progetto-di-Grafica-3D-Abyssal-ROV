#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// --- CLASSE CAMERA ---
// Implementa una telecamera "Fly/Free-look" a 6 gradi di libertà.
// Questa classe incapsula la matematica vettoriale (trigonometria sferica) 
// ed espone i metodi per generare la View Matrix per gli shader OpenGL.

class Camera {
public:
    // Attributi spaziali (Sistema di riferimento locale e globale)
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Angoli di Eulero (Rotazione attorno agli assi)
    float Yaw;
    float Pitch;

    // Impostazioni di navigazione
    float MovementSpeed;
    float MouseSensitivity;

    // Costruttore: Inizializza la telecamera a una specifica coordinata spaziale
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f));

    // Genera e restituisce la matrice di vista utilizzando la funzione glm::lookAt
    glm::mat4 GetViewMatrix();

    // Aggiorna la posizione nello spazio 3D in base agli input (WASD/Shift/Space) e al frame-rate
    void ProcessKeyboard(int direction, float deltaTime);

    // Traduce lo spostamento 2D del cursore del mouse in rotazione 3D (Pitch e Yaw)
    void ProcessMouseMovement(float xoffset, float yoffset);

private:
    // Ricalcola il sistema di assi locali (Front, Right, Up) a partire dai nuovi angoli di Eulero
    void updateCameraVectors();
};

#endif
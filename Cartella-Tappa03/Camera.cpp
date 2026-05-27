#include "Camera.h"
#include <algorithm> 

// COSTRUTTORE 
Camera::Camera(glm::vec3 position) {
    Position = position;
    WorldUp = glm::vec3(0.0f, 1.0f, 0.0f); // Imposta l'asse Y positivo come "Alto" assoluto del mondo
    Yaw = -90.0f; // Di default la telecamera guarda verso l'asse Z negativo (dentro lo schermo)
    Pitch = 0.0f;

    MovementSpeed = 5.0f;  
    MouseSensitivity = 0.1f;

    // Inizializza il sistema di riferimento locale prima del primo frame
    updateCameraVectors();  
}

// GENERAZIONE VIEW MATRIX 
glm::mat4 Camera::GetViewMatrix() {
    // glm::lookAt simula la telecamera applicando alla scena l'inverso delle trasformazioni.
    // Parametri richiesti: (Posizione occhio, Punto osservato, Vettore Alto)
    return glm::lookAt(Position, Position + Front, Up);
}

// ELABORAZIONE INPUT TASTIERA 
void Camera::ProcessKeyboard(int direction, float deltaTime) {
    // La velocità è moltiplicata per deltaTime per rendere il movimento fluido e indipendente dagli FPS
    float velocity = MovementSpeed * deltaTime;

    // Spostamento planare basato sui vettori direzionali locali
    if (direction == 0) Position += Front * velocity; // Avanti (W)
    if (direction == 1) Position -= Front * velocity; // Indietro (S)
    if (direction == 2) Position -= Right * velocity; // Sinistra (A)
    if (direction == 3) Position += Right * velocity; // Destra (D)

    // Spostamento verticale vincolato all'asse globale (Meccanica "Fly")
    if (direction == 4) Position += WorldUp * velocity; // Su (Spazio)
    if (direction == 5) Position -= WorldUp * velocity; // Giù (Shift)
}

// ELABORAZIONE INPUT MOUSE
void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // Constraint di sicurezza (Prevenzione del Gimbal Lock / Ribaltamento)
    // Impedisce alla telecamera di compiere una rotazione verticale superiore a 90 gradi.
    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    // Ricalcola i vettori spaziali applicando la nuova rotazione
    updateCameraVectors();
}

// MATEMATICA DEI VETTORI 
void Camera::updateCameraVectors() {
    // 1. Calcolo della nuova direzione frontale tramite Trigonometria Sferica
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

    // La normalizzazione è indispensabile per mantenere il vettore di lunghezza 1 (versore)
    Front = glm::normalize(front);
    
    // 2. Calcolo del vettore "Destra" locale tramite prodotto vettoriale (Cross Product)
    // Il prodotto vettoriale tra la vista frontale e l'alto globale genera un vettore ortogonale a entrambi.
    Right = glm::normalize(glm::cross(Front, WorldUp)); 
    Up    = glm::normalize(glm::cross(Right, Front));
}
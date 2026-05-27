# Abyssal-ROV - Guida Rapida (README)

Questo documento fornisce le istruzioni essenziali per compilare, lanciare e utilizzare tutte le tappe del progetto *Abyssal-ROV*. Per i dettagli teorici, architetturali e la dichiarazione degli strumenti utilizzati, si rimanda ai file `.md` specifici presenti nelle cartelle delle singole tappe.

## 1. Prerequisiti
Per compilare il progetto sul proprio computer, assicurarsi di avere installato:
* Un compilatore C++ (es. MinGW `ucrt64` su Windows o GCC/Clang su Linux/macOS).
* **CMake** (versione 3.5 o superiore).
* **Git** (necessario a CMake per il download automatico delle dipendenze SFML e GLM tramite `FetchContent`).


##  2. Istruzioni di Build (Metodo Unico)
Il progetto è configurato per compilare **tutte le tappe contemporaneamente** tramite un unico file `CMakeLists.txt` situato nella root del progetto.

Aprire il terminale nella root del progetto (dove si trova questo file) ed eseguire i seguenti due comandi:

1. **Configurazione e Generazione:**

   ```bash
   cmake -S . -B build

2. **Compilazione totale:**

    ```bash
    cmake --build build

Al termine di questo processo, tutti gli eseguibili (da Tappa01 a Tappa10) saranno generati e pronti all'uso all'interno della cartella build/.

## 3. Lancio degli Eseguibili:
Nessuna delle tappe richiede il passaggio di argomenti o risorse aggiuntive da linea di comando. Tutte le risorse (modelli, texture, font) vengono caricate dinamicamente dalla cartella Cartella-risorse/ tramite percorsi relativi preimpostati.

Per avviare una specifica tappa, eseguire da terminale il relativo file compilato.
Esempio per sistemi Windows:


```bash
.\build\Tappa01.exe
.\build\Tappa05.exe
.\build\Tappa10.exe
```

(**Nota**: su sistemi Unix/Linux/macOS utilizzare ./build/TappaXX)

## 4. Comandi e Interfaccia Utente (UI):
I controlli si sono evoluti parallelamente allo sviluppo del motore grafico. Di seguito lo schema dei comandi diviso per fasi di sviluppo.

### Tappa 01 (Setup Iniziale)

* **ESC:** Chiude istantaneamente la finestra.

### Tappe dalla 02 alla 09 (Esplorazione e Sviluppo Motore)
In queste tappe la simulazione parte immediatamente all'avvio dell'eseguibile. Il mouse è costantemente catturato al centro dello schermo per permettere la rotazione della visuale.

* **W / S:** Avanza / Indietreggia.
* **A / D:** Traslazione laterale.
* **Spazio / Shift Sinistro:** Emersione / Immersione.
* **Mouse:** Rotazione della telecamera.
* **ESC:** Chiude istantaneamente l'applicazione.
* **TAB:** Sblocco del mouse. Il cursore viene liberato e la telecamera viene messa in "pausa", permettendo di uscire dai confini della finestra per ridimensionarla o chiuderla tramite OS.


### Tappa 10 (Versione Finale)
La Tappa 10 introduce una Macchina a Stati (Menu, Gioco, Pausa, Game Over). Il tasto TAB è stato rimosso, poiché lo sblocco del mouse è ora gestito automaticamente dal motore di gioco in base allo stato in cui ci si trova.

* **INVIO:** Avvia la simulazione dal Menu Principale o riavvia la partita dalla schermata di Game Over.
* **W / S:** Avanza / Indietreggia.
* **A / D:** Traslazione laterale.
* **Spazio / Shift Sinistro:** Emersione / Immersione.
* **Mouse:** Rotazione della telecamera a 360 gradi (attiva esclusivamente nello stato PLAYING).
* **ESC:** Durante il gioco mette in Pausa la simulazione (Active Pause) e libera il cursore. Se premuto dal Menu Principale, chiude l'applicazione.
* **Gestione Cursore (Automatica):** Il mouse viene sbloccato in automatico in tutti gli stati di inattività (MENU, PAUSED, GAMEOVER), permettendo all'utente di uscire dai confini della finestra per ridimensionarla o chiuderla tramite i controlli nativi del sistema operativo.


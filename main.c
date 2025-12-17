#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#define MAX_PATOS 3
#include "graficos.h"
#include "simplecontroller.h"

#define JX 15
#define JY 2
#define BTN 4
#define MTR 13

typedef struct {
    int x, y;
    float velocidad_x;
    float velocidad_y;
    bool vivo;
    bool volando;
    int direccion;
    int frame;
} Pato;

Pato patos[MAX_PATOS];

typedef struct {
    int x, y;
    bool disparando;
    int balas;
    int tiempo_disparo;
} Mira;

typedef struct {
    int patos_cazados;
    int patos_escapados;
    int nivel;
    int puntos;
    bool juego_activo;
    int ronda_actual;
} EstadoJuego;

void inicializarPato(Pato *p, int nivel);
void actualizarMira(Mira *m, Board *esp32);
void verificarColisiones(Mira *m, Pato *patos, int num_patos);
bool colisionMiraPato(Mira *m, Pato *p);
void actualizarPato(Pato *p);

int main() {
    srand(time(NULL));

    printf("Inicializando ventana...\n");
    ventana.tamanioVentana(800, 600);
    ventana.tituloVentana("Duck Hunt");
    printf("Ventana creada correctamente\n");

    printf("Conectando a ESP32...\n");
    Board *esp32 = connectDevice("COM6", B115200);
    if (esp32 == NULL) {
        printf("Error: No se pudo conectar al dispositivo en COM6\n");
        printf("Verifica que el ESP32 esté conectado y el puerto sea correcto\n");
        ventana.cierraVentana();
        return -1;
    }
    printf("ESP32 conectado correctamente\n");

    printf("Cargando imágenes...\n");
    Imagen *pato_img[2];
    pato_img[0] = ventana.creaImagenConMascara("Imagenes/pato_1.bmp", "Imagenes/mascara_pato_1.bmp");
    pato_img[1] = ventana.creaImagenConMascara("Imagenes/pato_2.bmp", "Imagenes/mascara_pato_2.bmp");
    Imagen *pato_muerto = ventana.creaImagenConMascara("Imagenes/pato_muerto.bmp", "Imagenes/mascara_pato_muerto.bmp");
    Imagen *mira_img = ventana.creaImagenConMascara("Imagenes/mira.bmp", "Imagenes/mascara_mira.bmp");
    Imagen *fondo = ventana.creaImagen("Imagenes/fondo.bmp");

    if (!pato_img[0] || !pato_img[1] || !pato_muerto || !mira_img || !fondo) {
        printf("Error: No se pudieron cargar todas las imágenes\n");
        printf("Verifica que la carpeta 'Imagenes' exista y contenga todos los archivos:\n");
        printf("  - pato_1.bmp y mascara_pato_1.bmp\n");
        printf("  - pato_2.bmp y mascara_pato_2.bmp\n");
        printf("  - pato_muerto.bmp y mascara_pato_muerto.bmp\n");
        printf("  - mira.bmp y mascara_mira.bmp\n");
        printf("  - fondo.bmp\n");
        if (esp32) esp32->closeDevice(esp32);
        ventana.cierraVentana();
        return -1;
    }
    printf("Imágenes cargadas correctamente\n");

    // Configurar pines del ESP32
    esp32->pinMode(esp32, JX, INPUT);
    esp32->pinMode(esp32, JY, INPUT);
    esp32->pinMode(esp32, BTN, INPUT_PULLUP);
    esp32->pinMode(esp32, MTR, OUTPUT);

    EstadoJuego juego = {0, 0, 1, 0, true, 1};
    Mira mira = {400, 300, false, 3, 0};

    for (int i = 0; i < MAX_PATOS; i++) {
        inicializarPato(&patos[i], juego.nivel);
    }

    printf("Juego iniciado. Presiona ESC para salir.\n");

    int tecla = TECLAS.NINGUNA;

    while (tecla != TECLAS.ESCAPE && juego.juego_activo) {
        tecla = ventana.teclaPresionada();

        actualizarMira(&mira, esp32);

        for (int i = 0; i < MAX_PATOS; i++) {
            actualizarPato(&patos[i]);
        }

        bool todos_muertos = true;
        for (int i = 0; i < MAX_PATOS; i++) {
            if (patos[i].volando) todos_muertos = false;
        }

        if (todos_muertos && mira.balas == 0) {
            mira.balas = 3;
            juego.ronda_actual++;
            for (int i = 0; i < MAX_PATOS; i++) {
                inicializarPato(&patos[i], juego.nivel);
            }
        }

        ventana.limpiaVentana();

        ventana.muestraImagen(0, 0, fondo);

        for (int i = 0; i < MAX_PATOS; i++) {
            if (patos[i].vivo && patos[i].volando) {
                int frame = (patos[i].frame < 10) ? 0 : 1;
                ventana.muestraImagen(patos[i].x, patos[i].y, pato_img[frame]);
            } else if (!patos[i].vivo) {
                ventana.muestraImagen(patos[i].x, patos[i].y, pato_muerto);
            }
        }

        ventana.muestraImagen(mira.x - 25, mira.y - 25, mira_img);

        if (mira.tiempo_disparo > 0) {
            ventana.color(COLORES.AMARILLO);
            ventana.circuloRelleno(mira.x, mira.y, 50);
        }

        ventana.color(COLORES.BLANCO);
        char texto[50];
        sprintf(texto, "PUNTOS: %d", juego.puntos);
        ventana.texto1(20, 20, texto, 20, "Arial");

        sprintf(texto, "BALAS: %d", mira.balas);
        ventana.texto1(20, 50, texto, 20, "Arial");

        ventana.actualizaVentana();
        ventana.espera(16);
    }

    printf("Cerrando juego...\n");
    ventana.eliminaImagen(pato_img[0]);
    ventana.eliminaImagen(pato_img[1]);
    ventana.eliminaImagen(pato_muerto);
    ventana.eliminaImagen(mira_img);
    ventana.eliminaImagen(fondo);
    esp32->closeDevice(esp32);
    ventana.cierraVentana();

    printf("Juego cerrado. Puntos finales: %d\n", juego.puntos);
    return 0;
}

void inicializarPato(Pato *p, int nivel) {
    if (rand() % 2) {
        p->x = 0;
        p->direccion = 1;
    } else {
        p->x = ventana.anchoVentana();
        p->direccion = -1;
    }

    p->y = rand() % (ventana.altoVentana() - 200) + 100;
    p->velocidad_x = (2.0 + nivel * 0.5) * p->direccion;
    p->velocidad_y = (rand() % 3) - 1;  // -1, 0, o 1
    p->vivo = true;
    p->volando = true;
    p->frame = 0;
}

void actualizarPato(Pato *p) {
    if (!p->vivo || !p->volando) {
        if (p->vivo == false && p->y < ventana.altoVentana()) {
            p->y += 5;  // Gravedad
        }
        return;
    }

    p->x += p->velocidad_x;
    p->y += p->velocidad_y;

    if (p->y < 50 || p->y > ventana.altoVentana() - 150) {
        p->velocidad_y = -p->velocidad_y;
    }

    if (p->x < -50 || p->x > ventana.anchoVentana() + 50) {
        p->volando = false;
    }

    p->frame = (p->frame + 1) % 20;
}

void actualizarMira(Mira *m, Board *esp32) {
    float jx = esp32->analogRead(esp32, JX);
    float jy = esp32->analogRead(esp32, JY);
    bool btn = esp32->digitalRead(esp32, BTN);

    jx -= 0.51;
    jy -= 0.4;

    int vel_x = (jx >= 0) ? jx * 40 : jx * 42;
    int vel_y = (jy >= 0) ? jy * 42 : jy * 42;

    m->x += vel_x;
    m->y += vel_y;

    if (m->x < 0) m->x = 0;
    if (m->x > ventana.anchoVentana()) m->x = ventana.anchoVentana();
    if (m->y < 0) m->y = 0;
    if (m->y > ventana.altoVentana()) m->y = ventana.altoVentana();

    if (!btn && !m->disparando && m->balas > 0) {
        m->disparando = true;
        m->balas--;
        m->tiempo_disparo = 10;

        esp32->digitalWrite(esp32, MTR, true);

        verificarColisiones(m, patos, MAX_PATOS);
    } else if (btn) {
        m->disparando = false;
        esp32->digitalWrite(esp32, MTR, false);
    }

    if (m->tiempo_disparo > 0) {
        m->tiempo_disparo--;
    }
}

bool colisionMiraPato(Mira *m, Pato *p) {
    if (!p->vivo || !p->volando) return false;
    int radio_mira = 30;
    int radio_pato = 40;

    int dx = m->x - p->x;
    int dy = m->y - p->y;
    int distancia_sq = dx*dx + dy*dy;
    int suma_radios_sq = (radio_mira + radio_pato) * (radio_mira + radio_pato);

    return distancia_sq < suma_radios_sq;
}

void verificarColisiones(Mira *m, Pato *patos, int num_patos) {
    for (int i = 0; i < num_patos; i++) {
        if (colisionMiraPato(m, &patos[i])) {
            patos[i].vivo = false;
            patos[i].volando = false;
        }
    }
}
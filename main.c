#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "graficos.h"
#include "simplecontroller.h"

#define MAX_PATOS 3
#define JX 15
#define JY 2
#define BTN 4
#define MTR 13

#define PATO_ANCHO 60
#define PATO_ALTO  50
#define MIRA_ANCHO 50
#define MIRA_ALTO  50

typedef struct {
    float x, y;
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
void actualizarPato(Pato *p);
void actualizarMira(Mira *m, Board *esp32, EstadoJuego *juego);
void verificarColisiones(Mira *m, Pato *patos, int num_patos, EstadoJuego *juego);
bool colisionMiraPato(Mira *m, Pato *p);
void dibujarFondoAjustado(Imagen *img);
void dibujarEscalado(int x, int y, Imagen *img, int ancho, int alto);
void vueloSinusoidal(Pato *p, int tiempo);
void sumarPuntos(EstadoJuego *juego, int puntosextra);

int main() {
    srand(time(NULL));

    printf("Iniciando Ventana\n");
    ventana.tamanioVentana(700, 500);
    ventana.tituloVentana("Duck Hunt");

    printf("Cargando imagenes\n");
    Imagen *fondo2 = ventana.creaImagen("fondo2.bmp");
    Imagen *pato_img[2];
    pato_img[0] = ventana.creaImagenConMascara("pato_1.bmp", "mascara_pato_1.bmp");
    pato_img[1] = ventana.creaImagenConMascara("pato_2.bmp", "mascara_pato_2.bmp");
    Imagen *pato_muerto = ventana.creaImagenConMascara("pato_muerto.bmp", "mascara_pato_muerto.bmp");
    Imagen *mira_img = ventana.creaImagenConMascara("mira.bmp", "mascara_mira.bmp");

    if (!pato_img[0] || !mira_img) {
         printf("Error al cargar imgenes de patos o de mira""\n");
    }

    printf("Conectando a arduino\n");
    Board *esp32 = connectDevice("COM6", B115200);
    if (esp32 != NULL) {
        esp32->pinMode(esp32, JX, INPUT);
        esp32->pinMode(esp32, JY, INPUT);
        esp32->pinMode(esp32, BTN, INPUT_PULLUP);
        esp32->pinMode(esp32, MTR, OUTPUT);
        for(int i=0; i<10; i++) {
            esp32->analogRead(esp32, JX);
            ventana.espera(10);
        }
    } else {
        printf("Jugando en teclado\n");
    }

    EstadoJuego juego = {0, 0, 1, 0, true, 1};
    Mira mira = {ventana.anchoVentana()/2, ventana.altoVentana()/2, false, 3, 0};

    for (int i = 0; i < MAX_PATOS; i++) {
        inicializarPato(&patos[i], juego.nivel);
    }

    int tecla = TECLAS.NINGUNA;

    while (tecla != TECLAS.ESCAPE && juego.juego_activo) {
        tecla = ventana.teclaPresionada();

        if (esp32 != NULL) {
            actualizarMira(&mira, esp32, &juego);
        } else {
            if(tecla == TECLAS.DERECHA) mira.x += 10;
            if(tecla == TECLAS.IZQUIERDA) mira.x -= 10;
            if(tecla == TECLAS.ARRIBA) mira.y -= 10;
            if(tecla == TECLAS.ABAJO) mira.y += 10;

            if(tecla == TECLAS.ESPACIO && !mira.disparando && mira.balas > 0) {
                 mira.disparando = true;
                 mira.balas--;
                 mira.tiempo_disparo = 10;
                 verificarColisiones(&mira, patos, MAX_PATOS, &juego);
            } else if (tecla != TECLAS.ESPACIO) {
                 mira.disparando = false;
            }
            if (mira.tiempo_disparo > 0) mira.tiempo_disparo--;
        }

        if (mira.x < 0) mira.x = 0;
        if (mira.x > ventana.anchoVentana()) mira.x = ventana.anchoVentana();
        if (mira.y < 0) mira.y = 0;
        if (mira.y > ventana.altoVentana()) mira.y = ventana.altoVentana();

        bool hay_patos_activos = false;
        for (int i = 0; i < MAX_PATOS; i++) {
            actualizarPato(&patos[i]);
            if (patos[i].volando) hay_patos_activos = true;
        }

        if (!hay_patos_activos) {
            ventana.espera(1000);
            mira.balas = 3;
            juego.ronda_actual++;
            juego.nivel++;
            for (int i = 0; i < MAX_PATOS; i++) {
                inicializarPato(&patos[i], juego.nivel);
            }
        }
        ventana.limpiaVentana();

        dibujarFondoAjustado(fondo2);

        for (int i = 0; i < MAX_PATOS; i++) {
            if (patos[i].vivo && patos[i].volando) {
                int frame = (patos[i].frame < 10) ? 0 : 1;
                if(pato_img[frame])
                    dibujarEscalado((int)patos[i].x, (int)patos[i].y, pato_img[frame], PATO_ANCHO, PATO_ALTO);
            } else if (!patos[i].vivo) {
                if(pato_muerto)
                    dibujarEscalado((int)patos[i].x, (int)patos[i].y, pato_muerto, PATO_ANCHO, PATO_ALTO);
            }
        }

        if(mira_img) {
            dibujarEscalado(mira.x - MIRA_ANCHO/2, mira.y - MIRA_ALTO/2, mira_img, MIRA_ANCHO, MIRA_ALTO);
        }

        if (mira.tiempo_disparo > 0) {
            ventana.color(COLORES.ROJO);
            ventana.circulo(mira.x, mira.y, 5);
        }

        ventana.color(COLORES.BLANCO);
        char texto[60];
        sprintf(texto, "NIVEL: %d  PUNTOS: %d", juego.nivel, juego.puntos);
        ventana.texto(10, 10, texto);

        sprintf(texto, "BALAS: %d", mira.balas);
        ventana.texto(10, 30, texto);

        ventana.actualizaVentana();
        ventana.espera(16);
    }

    ventana.cierraVentana();
    return 0;
}

void sumarPuntos(EstadoJuego *juego, int puntosextra) {
    juego->puntos +=puntosextra;
    juego->patos_cazados++;
    if (juego->patos_cazados % 10 == 0) {
        juego->patos_cazados+=1000;
    }
}

void dibujarEscalado(int x, int y, Imagen *img, int ancho, int alto) {
    if (img != NULL) {
        ventana.muestraImagenEscalada(x, y, ancho, alto, img);
    }
}

void dibujarFondoAjustado(Imagen *img) {
    if (img == NULL) return;
    int ancho_ventana = ventana.anchoVentana();
    int alto_ventana = ventana.altoVentana();
    ventana.muestraImagenEscalada(0, 0, ancho_ventana, alto_ventana, img);
}

void inicializarPato(Pato *p, int nivel) {
    int w = ventana.anchoVentana();
    int h = ventana.altoVentana();

    if (rand() % 2) {
        p->x = -PATO_ANCHO;
        p->direccion = 1;
    } else {
        p->x = w;
        p->direccion = -1;
    }

    p->y = rand() % (h - 200) + 50;

    p->velocidad_x = (2.0 + (nivel * 0.5)) * p->direccion;
    p->velocidad_y = ((rand() % 30) - 15) / 10.0;

    p->vivo = true;
    p->volando = true;
    p->frame = 0;
}

void actualizarPato(Pato *p) {
    int h = ventana.altoVentana();
    int w = ventana.anchoVentana();

    if (!p->vivo) {
        if (p->y < h) {
            p->y += 6;
        } else {
            p->volando = false;
        }
        return;
    }

    if (!p->volando) return;

    p->x += p->velocidad_x;
    p->y += p->velocidad_y;

    if (p->y < 0 || p->y > h - 150) {
        p->velocidad_y = -p->velocidad_y;
    }

    if (p->x < -PATO_ANCHO - 50 || p->x > w + 50) {
        p->volando = false;
    }

    p->frame = (p->frame + 1) % 20;
}

void actualizarMira(Mira *m, Board *esp32, EstadoJuego *juego) {
    if (esp32 == NULL) return;

    float jx = esp32->analogRead(esp32, JX);
    float jy = esp32->analogRead(esp32, JY);
    bool btn = esp32->digitalRead(esp32, BTN);

    jx -= 0.50;
    jy -= 0.50;

    if (fabs(jx) < 0.05) jx = 0;
    if (fabs(jy) < 0.05) jy = 0;

    int vel_x = jx * 30;
    int vel_y = jy * 30;

    m->x += vel_x;
    m->y += vel_y;

    if (!btn && !m->disparando && m->balas > 0) {
        m->disparando = true;
        m->balas--;
        m->tiempo_disparo = 10;
        esp32->digitalWrite(esp32, MTR, true);

        verificarColisiones(m, patos, MAX_PATOS, juego);
    }
    else if (btn) {
        m->disparando = false;
        esp32->digitalWrite(esp32, MTR, false);
    }
}

void verificarColisiones(Mira *m, Pato *patos, int num_patos, EstadoJuego *juego) {
    for (int i = 0; i < num_patos; i++) {
        if (colisionMiraPato(m, &patos[i])) {
            patos[i].vivo = false;
            sumarPuntos(juego, 100);
        }
    }
}

bool colisionMiraPato(Mira *m, Pato *p) {
    if (!p->vivo || !p->volando) return false;

    int radio_mira = MIRA_ANCHO / 2;
    int radio_pato = PATO_ANCHO / 2;

    int centro_pato_x = (int)p->x + (PATO_ANCHO / 2);
    int centro_pato_y = (int)p->y + (PATO_ALTO / 2);

    int dx = m->x - centro_pato_x;
    int dy = m->y - centro_pato_y;

    int distancia_sq = (dx * dx) + (dy * dy);
    int suma_radios = (radio_mira + radio_pato) - 10;

    return distancia_sq < (suma_radios * suma_radios);
}


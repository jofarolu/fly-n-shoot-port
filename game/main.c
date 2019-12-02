/*****************************************************************************
*                     INSTITUTO POLITÉCNICO NACIONAL
* UNIDAD PROFESIONAL INTERDISCIPLINARIA EN INGENIERÍA Y TECNOLOGÍAS AVANZADAS
*
*                   SISTEMAS OPERATIVOS EN TIEMPO REAL.
*                             Periodo 20/1
*
* Port de juego Fly'n'Shoot (Quantum Leaps) para GNU/Linux con ncurses
*
* Autores de port:
* José Fausto Romero Lujambio
* Pastor Alan Rodríguez Echeverría
*
******************************************************************************
* Product: "Fly'n'Shoot" game example
* Last Updated for Version: 4.5.05
* Date of the Last Update:  Mar 22, 2013
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2013 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Contact information:
* Quantum Leaps Web sites: http://www.quantum-leaps.com
*                          http://www.state-machine.com
* e-mail:                  info@quantum-leaps.com
*****************************************************************************/
#include <unistd.h>
#include <pthread.h>

#include "qp_port.h"
#include "bsp.h"
#include "game.h"
#include "video.h"

/* Port de ncurses */
extern int screenWidth,screenHeight;

volatile int thread_signal;
static pthread_t event_thread, kernel_thread;

void print_exit_screen(void);

/* Local-scope objects -----------------------------------------------------*/
static QEvt const * l_missileQueueSto[2];
static QEvt const * l_shipQueueSto[3];
static QEvt const * l_tunnelQueueSto[GAME_MINES_MAX + 5];

static union SmallEvents {
    void   *e0;                                       /* minimum event size */
    uint8_t e1[sizeof(QEvt)];
    /* ... other event types to go into this pool */
} l_smlPoolSto[10];                     /* storage for the small event pool */

static union MediumEvents {
    void   *e0;                                       /* minimum event size */
    uint8_t e1[sizeof(ObjectPosEvt)];
    uint8_t e2[sizeof(ObjectImageEvt)];
    uint8_t e3[sizeof(MineEvt)];
    uint8_t e4[sizeof(ScoreEvt)];
    /* ... other event types to go into this pool */
} l_medPoolSto[2*GAME_MINES_MAX + 8];  /* storage for the medium event pool */

static QSubscrList    l_subscrSto[MAX_PUB_SIG];

/*..........................................................................*/
void *kernel_thread_handler(void *status) {
    int *thread_status = (int *) status;
    for (;;) {
        if (*thread_status == 1)
            break;

        // ejecutar bucle de eventos del kernel QP
        QF_run_event_loop();

        // ESTA LÍNEA ES CRÍTICA PARA EL FUNCIONAMIENTO DEL KERNEL
        usleep(10); // retardo de procesamiento de kernel
    }
    return NULL;
}
/*..........................................................................*/
void *event_thread_handler(void *status) {
    int *thread_status = (int *) status;
    double period = 1.0/BSP_TICKS_PER_SEC;
    for (;;) {
        // procesar señales de control
        if (*thread_status == 1)
            break;

        // procesar entrada de teclado
        if (!keyboard_support())
            break;

        // evento de reloj
        timer_event();

        // dibujar pantalla
        Video_render();
        usleep(period * 1e6);
    }
    print_exit_screen();
    return NULL;
}
/*..........................................................................*/
void print_exit_screen(void) {
    // Pantalla de salida de aplicación
    Video_clearScreen(VIDEO_BGND_BLACK);
    Video_printStrAt(2, 19, VIDEO_FGND_WHITE, "Port realizado por:");
    Video_printStrAt(2, 20, VIDEO_FGND_WHITE, "Jose Fausto Romero Lujambio");
    Video_printStrAt(2, 21, VIDEO_FGND_WHITE, "Pastor Alan Rodriguez Echeverria");
    Video_printStrAt(2, 22, VIDEO_FGND_YELLOW, "Juego terminado. Presione cualquier tecla para salir...");
    Video_render();
}
/*..........................................................................*/
int main(int argc, char *argv[]) {
    /* inicializar ncurses */
    initscr(); // iniciar pantalla
    curs_set(0); // ocultar cursor

    // configurar teclado
    timeout(0); cbreak(); noecho();
    nodelay(stdscr, TRUE); scrollok(stdscr, TRUE);

    // configurar colores y limpiar pantalla
    init_color_pairs();
    clear(); refresh();

    /* explicitly invoke the active objects' ctors... */
    Missile_ctor();
    Ship_ctor();
    Tunnel_ctor();

    BSP_init(argc, argv);           /* initialize the Board Support Package */

    QF_init();     /* initialize the framework and the underlying RT kernel */

                                           /* initialize the event pools... */
    QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(l_smlPoolSto[0]));
    QF_poolInit(l_medPoolSto, sizeof(l_medPoolSto), sizeof(l_medPoolSto[0]));

    QF_psInit(l_subscrSto, Q_DIM(l_subscrSto));   /* init publish-subscribe */

                             /* send object dictionaries for event pools... */
    QS_OBJ_DICTIONARY(l_smlPoolSto);
    QS_OBJ_DICTIONARY(l_medPoolSto);

               /* send signal dictionaries for globally published events... */
    QS_SIG_DICTIONARY(TIME_TICK_SIG,      0);
    QS_SIG_DICTIONARY(PLAYER_TRIGGER_SIG, 0);
    QS_SIG_DICTIONARY(PLAYER_QUIT_SIG,    0);
    QS_SIG_DICTIONARY(GAME_OVER_SIG,      0);

                                             /* start the active objects... */
    QActive_start(AO_Missile,
                  1,                                            /* priority */
                  l_missileQueueSto, Q_DIM(l_missileQueueSto), /* evt queue */
                  (void *)0, 0,                      /* no per-thread stack */
                  (QEvt *)0);                  /* no initialization event */
    QActive_start(AO_Ship,
                  2,                                            /* priority */
                  l_shipQueueSto,    Q_DIM(l_shipQueueSto),    /* evt queue */
                  (void *)0, 0,                      /* no per-thread stack */
                  (QEvt *)0);                  /* no initialization event */
    QActive_start(AO_Tunnel,
                  3,                                            /* priority */
                  l_tunnelQueueSto,  Q_DIM(l_tunnelQueueSto),  /* evt queue */
                  (void *)0, 0,                      /* no per-thread stack */
                  (QEvt *)0);                  /* no initialization event */

    /* hilos de programa */
    pthread_create(&kernel_thread, NULL, kernel_thread_handler, (void *)&thread_signal);
    pthread_create(&event_thread, NULL, event_thread_handler, (void *)&thread_signal);
    pthread_join(kernel_thread, NULL);
    pthread_join(event_thread, NULL);

    // esperar la entrada del usuario
    timeout(-1); getch(); clear(); refresh();
    endwin(); // terminar ncurses
    return 0;
}

#ifndef ANIM_H
#define ANIM_H

#include <stdbool.h>

// Lookup tables ease-out-cubic e ease-in-out (arrays mutáveis, gerados no boot)
extern float EASE_OUT[256];
extern float EASE_IN_OUT[256];

// Estrutura de animação por valor
typedef struct {
    float value;     // valor atual interpolado
    float target;    // valor destino (0.0 a 1.0)
    float speed;     // velocidade de aproximação (0.05 a 0.25)
    bool  finished;  // true quando |value - target| < 0.001
} Anim;

// Inicializar animações - gera as lookup tables EASE_OUT e EASE_IN_OUT
void animInit(void);

// Atualizar uma animação (chamar 1x por frame)
void animStep(Anim* a);

// Retornar valor com easing aplicado via lookup table
float animEasedOut(Anim* a);
float animEasedInOut(Anim* a);

// Definir valor alvo (dispara animação)
void animTo(Anim* a, float target);

// Inicializar animação já no valor alvo (sem animação inicial)
void animSet(Anim* a, float value, float speed);

#endif // ANIM_H

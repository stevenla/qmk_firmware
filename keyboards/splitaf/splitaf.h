#ifndef SPLITAF_H
#define SPLITAF_H

#include "quantum.h"

// see https://imgur.com/a/NraS6
#define KEYMAP( \
  k00,  k01,  k02,  k03,  k04,  k05,  k06,      k07,  k08,  k09,  k0A,  k0B,  k0C,  k0D,  k0E,  k0F,  \
  k10,        k12,  k13,  k14,  k15,  k16,      k17,  k18,  k19,  k1A,  k1B,  k1C,  k1D,  k1E,  k1F,  \
  k20,        k22,  k23,  k24,  k25,  k26,      k27,  k28,  k29,  k2A,  k2B,  k2C,  k2D,        k2F,  \
  k30,        k32,  k33,  k34,  k35,  k36,      k37,  k38,  k39,  k3A,  k3B,  k3C,        k3E,  k3F,  \
  k40,  k41,  k42,  k43,        k45,                  k48,  k49,        k4B,  k4C,  k4D,  k4E,  k4F  \
)  \
{ \
   {k00,   k01,   k02,   k03,   k04,   k05,   k06,   k07,   k08,   k09,   k0A,   k0B,   k0C,   k0D,   k0E,   k0F},   \
   {k10,   KC_NO, k12,   k13,   k14,   k15,   k16,   k17,   k18,   k19,   k1A,   k1B,   k1C,   k1D,   k1E,   k1F},   \
   {k20,   KC_NO, k22,   k23,   k24,   k25,   k26,   k27,   k28,   k29,   k2A,   k2B,   k2C,   k2D,   KC_NO, k2F},   \
   {k30,   KC_NO, k32,   k33,   k34,   k35,   k36,   k37,   k38,   k39,   k3A,   k3B,   k3C,   KC_NO, k3E,   k3F},   \
   {k40,   k41,   k42,   k43,   KC_NO, k45,   KC_NO, KC_NO, k48,   k49,   KC_NO, k4B,   k4C,   k4D,   k4E,   k4F},   \
}

#endif

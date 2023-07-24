#ifndef NUNCHUK_H_INCLUDED
#define NUNCHUK_H_INCLUDED

#include "wiiuse_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

int nunchuk_handshake(struct wiimote_t* wm, struct nunchuk_t* nc, ubyte* data, uword len);
void nunchuk_disconnected(struct wiimote_t* wm, struct nunchuk_t* nc);
int nunchuk_event(struct wiimote_t* wm, struct nunchuk_t* nc, ubyte* msg, ubyte len);

#ifdef __cplusplus
}
#endif

#endif

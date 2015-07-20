#include "struct.h"


int initInputDevice (char*, char*, StateMachine*);

int frameReadLoop (StateMachine*);
int rawPacket_nextFree (StateMachine*);

int rawPacket_nextWritten (StateMachine*);

int rawPacket_markWritten (StateMachine*, int );
int rawPacket_markRead (StateMachine*, int );

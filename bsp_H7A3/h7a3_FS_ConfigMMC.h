#ifndef h7a3_FS_ConfigMMC_H
#define h7a3_FS_ConfigMMC_H

#include "FS_OS.h"
void FS_X_OS_Lock(unsigned LockIndex);

void FS_X_OS_Init(unsigned NumLocks);
void FS_X_OS_Unlock(unsigned LockIndex);

#endif


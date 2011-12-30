/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3D_MODEL_LOG_H
#define _3D_MODEL_LOG_H

// Enabling the "Model" and "Piece" log subsystems will help debug model loading
#define LOG_SECTION_MODEL "Model"
#define LOG_SECTION_PIECE "Piece"

#include "System/Log/ILog.h"

LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_MODEL)
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_PIECE)

#endif // _3D_MODEL_LOG_H

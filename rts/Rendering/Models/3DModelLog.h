/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DMODELLOG_H_
#define _3DMODELLOG_H_

#include "System/LogOutput.h"

// Enabling the "Model" and "Piece" log subsystems will help debug model loading. "Model-detail" is extremely verbose.
static CLogSubsystem LOG_MODEL("Model");
static CLogSubsystem LOG_MODEL_DETAIL("Model-detail");
static CLogSubsystem LOG_PIECE("Piece");
static CLogSubsystem LOG_PIECE_DETAIL("Piece-detail");

#endif // _3DMODELLOG_H_

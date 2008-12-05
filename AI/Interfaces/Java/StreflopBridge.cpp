/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StreflopBridge.h"

#include "streflop_cond.h"
/*
#include <stdbool.h>	// bool, true, false
#include <string.h>	// strlen(), strcat(), strcpy()
#include <stdlib.h>	// malloc(), calloc(), free()
*/

void streflopBridge_setSpringFPUSettings() {
	streflop_init<Simple>();
}

void streflopBridge_setJavaFPUSettings() {
	streflop_init<Double>();
}

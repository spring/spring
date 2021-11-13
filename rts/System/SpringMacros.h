#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FILEPOS __FILE__ ":" TOSTRING(__LINE__)
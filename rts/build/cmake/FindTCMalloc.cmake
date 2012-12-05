# This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

# Find TCMalloc (part of Google's perftools)

SET(TCMALLOC_NAMES ${TCMALLOC_NAMES} tcmalloc)
FIND_LIBRARY(TCMALLOC_LIBRARY NAMES ${TCMALLOC_NAMES})

MARK_AS_ADVANCED(TCMALLOC_LIBRARY)

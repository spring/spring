!ifdef TEST_BUILD
	!define PRODUCT_NAME "Spring - Test Build"
!else
	!define PRODUCT_NAME "Spring"
!endif

!define PRODUCT_VERSION "${VERSION_TAG}"
!define SP_BASENAME "spring_${PRODUCT_VERSION}"
!define PRODUCT_PUBLISHER "Spring team"
!define PRODUCT_WEB_SITE "http://www.springrts.com"

; This is required, and has to be a string constant
; (can not contain defines like ${BLA})
VIProductVersion "0.0.0.1"

VIAddVersionKey "ProductName"      "Spring RTS engine"
VIAddVersionKey "CompanyName"      "Spring community"
VIAddVersionKey "LegalCopyright"   "GNU GPL V2 and V3"
VIAddVersionKey "FileDescription"  "Engine launcher executable"
VIAddVersionKey "FileVersion"      "${VERSION_TAG}"


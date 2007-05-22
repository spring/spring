!ifdef TEST_BUILD
 !define PRODUCT_NAME "Spring - Test Build"
 !define PRODUCT_VERSION "0.74b3+r${REVISION}"
 !define SP_BASENAME "spring_${PRODUCT_VERSION}_dev"
!else
 !define PRODUCT_NAME "Spring"
 !define PRODUCT_VERSION "0.74b3"
 !define SP_BASENAME "spring_${PRODUCT_VERSION}"
!endif

!define PRODUCT_PUBLISHER "The Spring team"
!define PRODUCT_WEB_SITE "http://spring.clan-sy.com"

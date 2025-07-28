vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Znurre/gd_parser
    REF main
    SHA512 d23723540027b2aa15e1558f737a79369713e3eef2a4e867a0550c78b18a4a30d9f7b9c0b2bab751616b5dff3930bc0679a397159cbb3b302cb69e1e4f2e262d
)

file(GLOB HEADERS
  "${SOURCE_PATH}/*.hpp"
)

foreach(HEADER IN LISTS HEADERS)
    file(INSTALL "${HEADER}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/gd_parser")
endforeach()

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

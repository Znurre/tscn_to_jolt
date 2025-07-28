vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Znurre/havoc
    REF main
    SHA512 1da164b3a854d5eba25ccda76ac5d350b89e0bac091c69ee07250473f0e6ca230578b9fd43c20c8a8bfa7f6646e68f388e2527dda72abf13bd649ab93291c17e
)

file(GLOB HEADERS
  "${SOURCE_PATH}/*.hpp"
)

foreach(HEADER IN LISTS HEADERS)
    file(INSTALL "${HEADER}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/havoc")
endforeach()

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

# Command-line tools (built only when BUILD_TOOLS=ON; included from the root).
#
# They link libtether, which transitively provides its public include directory
# and framework dependencies, so no extra include/link setup is needed here.

add_executable(attach-dmg attach_dmg.c)
target_link_libraries(attach-dmg PRIVATE tether)

add_executable(detach-dmg detach_dmg.c)
target_link_libraries(detach-dmg PRIVATE tether)

install(TARGETS attach-dmg detach-dmg
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../man/attach-dmg.1
    ${CMAKE_CURRENT_SOURCE_DIR}/../man/detach-dmg.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

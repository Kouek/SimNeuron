set(TARGET_NAME "SimWorm")

file(GLOB SRC "*.cpp")
message(STATUS "SRC: ${SRC}")
file(GLOB HEADER_ONLY_SRC "*.hpp")
message(STATUS "HEADER_ONLY_SRC: ${HEADER_ONLY_SRC}")
file(GLOB QT_UI "*.ui")
message(STATUS "QT_UI: ${QT_UI}")

add_executable(
	${TARGET_NAME}
	${QT_UI}
	${SRC} ${HEADER_ONLY_SRC}
	"${THIRDPARTY_DIR}/glad/src/glad.c"
)
target_link_libraries(
	${TARGET_NAME}
	PRIVATE
	${Qt5_LIBS}
	"glm::glm"
)


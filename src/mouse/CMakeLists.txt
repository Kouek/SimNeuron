set(TARGET_NAME "SimMouse")

add_executable(
	${TARGET_NAME}
	"main.cpp"
	"${THIRDPARTY_DIR}/glad/src/glad.c"
)
target_link_libraries(
	${TARGET_NAME}
	PRIVATE
	${Qt5_LIBS}
)


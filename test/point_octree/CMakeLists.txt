set(TARGET_NAME "TestPointOctree")

message(STATUS "Building Target: ${TARGET_NAME}")
file(GLOB SRC "*.cpp")

add_executable(
	${TARGET_NAME}
	${SRC}
)
target_link_libraries(
	${TARGET_NAME}
	"glm::glm"
)

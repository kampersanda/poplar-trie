file(GLOB HEADERS "poplar/*.hpp")
install(FILES poplar.hpp DESTINATION include)
install(FILES ${HEADERS} DESTINATION include/poplar)

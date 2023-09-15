CXX = g++
CXX_FLAGS = -std=c++23 -g
EXEC = rsa
RSA = rsa.cpp

${EXEC}: ${RSA}
	${CXX} ${CXX_FLAGS} ${RSA} -o ${EXEC}


.PHONY: clean
clean:
	rm -rf ${EXEC}

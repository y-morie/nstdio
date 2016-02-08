#!/bin/sh

FE_EXE=./fe
NODE_EXE=./node
PC_EXE=./pc

PORT_NODE=44444
PORT_PC=33333

CHARS=test_local

${FE_EXE} ${PORT_NODE} ${PORT_PC} &
${NODE_EXE} localhost ${PORT_NODE} ${CHARS} &
${PC_EXE} localhost ${PORT_PC}




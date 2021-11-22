#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH/:$HOME/lib-nstdio/lib
SV_EXE=./test-perfd
CL_EXE=./test-perfc

PORT=44444

${SV_EXE} ${PORT} &
${CL_EXE} localhost ${PORT} 

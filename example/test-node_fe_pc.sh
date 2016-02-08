#!/bin/sh

PROG_DIR_FE=/home/morie/NSTDIO/example/
PROG_DIR_NODE=/home/morie/NSTDIO/example/
PROG_DIR_PC=/Volumes/Home/ymorie/NSTDIO/example/

FE_EXE=fe
NODE_EXE=node
PC_EXE=pc

USER_FE=morie
USER_NODE=morie

FE_NAME_FPC=$1
NODE_NAME=$2
FE_NAME_FNODE=$3

PORT_NODE=44444
PORT_PC=33333

CHARS=$4

ssh -f -N -L ${PORT_PC}:localhost:${PORT_PC} ${USER_FE}@${FE_NAME_FPC}
ssh -f ${USER_FE}@${FE_NAME_FPC} ${PROG_DIR_FE}/${FE_EXE} ${PORT_NODE} ${PORT_PC}
ssh -f ${USER_FE}@${FE_NAME_FPC} ssh ${USER_NODE}@${NODE_NAME} ${PROG_DIR_NODE}/${NODE_EXE} ${FE_NAME_FNODE} ${PORT_NODE} ${CHARS}
sleep 1s
${PROG_DIR_PC}/${PC_EXE} localhost ${PORT_PC}
sleep 1s
PID=`ps aux | awk '/ssh -f/ {print $2;}'`
echo $PID
kill $PID


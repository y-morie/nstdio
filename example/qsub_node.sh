#!/bin/sh
#PBS -l nodes=4:ppn=1
#PBS -q Q1
#PBS -j oe

PROG_DIR_NODE=$HOME/NSTDIO/example
NODE_EXE=node
FE_NODE=acecls
NODE_PORT=44444
CHARS=qsub_node_test

${PROG_DIR_NODE}/${NODE_EXE} ${FE_NODE} ${NODE_PORT} ${CHARS}
exit 0

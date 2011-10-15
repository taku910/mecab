#!/bin/sh

cd cost-train

CORPUS=ipa.train
TEST=ipa.test
MODEL=model-ipadic
SEEDDIR=seed
FREQ=1
C=1.0
EVAL="0 1 2 4"

DIR=../../src
#DIR=/usr/local/libexec/mecab
RMODEL=${MODEL}.c${C}.f${FREQ}
DICDIR=${RMODEL}.dic

for algo in crf hmm
do
mkdir ${DICDIR}
cp -f ${SEEDDIR}/rewrite.def.${algo} ${SEEDDIR}/rewrite.def
cp -f test.gld.${algo} test.gld
cp -f dic.gld.${algo}  dic.gld
${DIR}/mecab-dict-index -d ${SEEDDIR} -o ${SEEDDIR}
${DIR}/mecab-cost-train -a ${algo} -c ${C} -d ${SEEDDIR} -f ${FREQ} ${CORPUS} ${RMODEL}.model
${DIR}/mecab-dict-gen   -a ${algo} -d ${SEEDDIR} -m ${RMODEL}.model -o ${DICDIR}
${DIR}/mecab-dict-index -d ${DICDIR} -o ${DICDIR}
${DIR}/mecab-test-gen < ${TEST} | ${DIR}/mecab -r /dev/null -d ${DICDIR}  > ${RMODEL}.result
${DIR}/mecab-system-eval -l "${EVAL}" ${RMODEL}.result ${TEST} | tee ${RMODEL}.score

diff test.gld ${RMODEL}.result
if [ "$?" != "0" ]
then
  echo "runtests faild in $dir"
  exit -1
fi;

diff dic.gld ${DICDIR}/dic.csv
if [ "$?" != "0" ]
then
  echo "runtests faild in $dir"
  exit -1
fi;

rm -fr ${DICDIR}
rm -fr ${RMODEL}*
rm -fr ${SEEDDIR}/*.dic
rm -fr ${SEEDDIR}/*.bin
rm -fr ${SEEDDIR}/*.dic
rm -fr test.gld

done

exit 0

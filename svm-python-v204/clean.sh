rm *feats.b.txt > /dev/null 2> /dev/null
rm nohup.out > /dev/null 2> /dev/null
for i in `seq 1 4`
do
  rm -rf fold$i/imodels/* > /dev/null 2> /dev/null
  rm -rf fold$i/logs/* > /dev/null 2> /dev/null
  rm -rf fold$i/models/* > /dev/null 2> /dev/null
  rm -rf fold$i/pred/* > /dev/null 2> /dev/null
done

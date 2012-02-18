rm *feats.b.txt > /dev/null 2> /dev/null
rm nohup.out > /dev/null 2> /dev/null
for i in `seq 1 4`
do
  rm -rf fold$i > /dev/null 2> /dev/null
done

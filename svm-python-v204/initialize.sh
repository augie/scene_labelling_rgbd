rm data_nodefeats.*.txt data_edgefeats.*.txt datas_*.txt > /dev/null 2> /dev/null
perl ../../filter.pl data_nodefeats.txt
perl ../../filter.pl data_edgefeats.txt
matlab -nodesktop -nosplash -r binfeats
cat header_data_nodefeats.txt temp_data_nodefeats.b.txt > data_nodefeats.b.txt
cat header_data_edgefeats.txt temp_data_edgefeats.b.txt > data_edgefeats.b.txt
rm temp_data_* header_data_* > /dev/null 2> /dev/null
perl ../../format.pl data_nodefeats.b.txt data_edgefeats.b.txt labelmap.txt 1

for i in `seq 1 4`
do
  mkdir fold$i > /dev/null 2> /dev/null
  mkdir fold$i/pred/ > /dev/null 2> /dev/null
  mkdir fold$i/logs/ > /dev/null 2> /dev/null
  mkdir fold$i/models/ > /dev/null 2> /dev/null
  mkdir fold$i/imodels/ > /dev/null 2> /dev/null
  touch fold$i/test$i
  touch fold$i/train$i
  rm fold$i/pred/* >/dev/null 2> /dev/null
  rm fold$i/logs/* > /dev/null 2> /dev/null
  rm fold$i/models/* > /dev/null 2> /dev/null
  rm fold$i/imodels/* > /dev/null 2> /dev/null
done

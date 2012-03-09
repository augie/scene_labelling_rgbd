#!/bin/bash

data=$1
loss=micro
lmethod=objassoc
cmethod=sum1.IP
objmapfile=$data/objectmap.txt
c=0.001
e=0.01

for i in `seq 1 4`
do
  suffix=c$c.e$e.$lmethod
  modelFile=model.$suffix
  modelFolder=$data/fold$i/models
  sh run_svm.sh $c $e $i $modelFile $modelFolder $suffix $cmethod $lmethod $loss $objmapfile $data
done

echo "processes completed!"
perl get_avg_pr.pl out.$cmethod.$modelFile $data > $data/avg_pr.$cmethod.$modelFile
method=$suffix.$cmethod
perl get_confusion_matrix.pl out.$cmethod.$modelFile $method $data > $data/confusionM.$method

rm runinfo
echo $HOSTNAME >> runinfo
pwd >> runinfo

echo "method : $method" >> runinfo
echo "loss: $loss" >> runinfo

echo "errors:" >> runinfo
cat errfile >> runinfo
rm errfile

echo "" >> runinfo
echo "~~~~~~~~~~~~~~~" >> runinfo
echo "" >> runinfo
echo "" >> runinfo
cat $data/avg_pr.$cmethod.$modelFile >> runinfo
echo "" >> runinfo
echo "~~~~~~~~~~~~~~~" >> runinfo
echo "" >> runinfo
echo "" >> runinfo
cat $data/confusionM.$method >> runinfo

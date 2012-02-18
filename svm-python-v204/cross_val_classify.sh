method=sum1.IP
for i in `seq 1 4`
do
  modelFile=`ls -lrt fold$i/imodels/model.c4.0.m* | cut -f 3 -d '/'| tail -1`
  modelFolder=fold$i/imodels
  echo "out.$method.$modelFile" > fold$i/lastout.txt
  mkdir fold$i/pred
  ../svm_python_classify --m svmstruct_mrf  fold$i/test$i $modelFolder/$modelFile fold$i/pred/pred.$method.$modelFile > fold$i/pred/out.$method.$modelFile &
  sleep 60
done

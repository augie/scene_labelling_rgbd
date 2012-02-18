folderName=`pwd | cut -f 5 -d '/'`
echo "folder is $folderName"

for i in `seq 1 4`
do
  sed 's/opt\/ros\/unstable\/stacks/critical\/scene_labelling/' fold$i/train$i >temp
  mv temp fold$i/train$i
  sed 's/opt\/ros\/unstable\/stacks/critical\/scene_labelling/' fold$i/test$i >temp
  mv temp fold$i/test$i
done

sed 's/opt\/ros\/unstable\/stacks/critical\/scene_labelling/' run.sh >temp
mv temp run.sh

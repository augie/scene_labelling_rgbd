num=0

for file in `ls transformed*.pcd`
do
  num=`expr $num + 1`
  echo $num
  echo $file
  lfile=` echo $file | sed s/transformed_// `
  labeledfile='labeled/'$lfile
  ls -lh $labeledfile
  bfile=`echo $lfile | sed 's/labeled_\(.*\)_segmented_xyzn.*/\1/'`
  core=`echo $lfile | sed 's/labeled_\(.*\)_segmented_xyzn.*/\1/'`
  bagfile="rgbdslamOut/$bfile.bag.stitched.bag"
  echo  $bagfile
  ls -l $bagfile
  rosrun scene_processing compute_all_features $file $num $labeledfile $bagfile #$num_bins_color $num_bins_shape
done

#include "CombineUtils.h"
#include<set>
using namespace std;

int
main(int argc, char** argv)
{
    //  ros::init(argc, argv,"hi");

    rosbag::Bag bag;
    std::cerr << "opening " << argv[1] << std::endl;
    bag.open(argv[1], rosbag::bagmode::Read);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr  cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB> ());// cloud_transformed(new pcl::PointCloud<PointT > ());
    pcl::PointCloud<PointT>::Ptr  cloud_normal(new pcl::PointCloud<PointT > ());// cloud_transformed(new pcl::PointCloud<PointT > ());
    pcl::PointCloud<pcl::PointXYGRGBCam>::Ptr final_cloud(new pcl::PointCloud<pcl::PointXYGRGBCam> ());

    int tf_count = 0;
    int pcl_count = 0;
    int numOccluded=0;
    int numOcclRepudiated=0;

    std::vector<TransformG> transformsG;
    std::vector<pcl::PointCloud<PointT>::Ptr> pointClouds;
    std::vector<pcl::KdTreeFLANN<PointT>::Ptr> searchTrees;
    pcl::PCDWriter writer;
  std::vector<int> k_indices;
  std::vector<float> k_distances;

    pcl_ros::BAGReader reader;
    if (!reader.open(argv[1], "/rgbdslam/my_clouds"))
    {
        ROS_ERROR("Couldn't read file ");
        return (-1);
    }
    sensor_msgs::PointCloud2ConstPtr cloud_blob, cloud_blob_prev;
    pcl::PointCloud<pcl::PointXYZRGB> inp_cloud;

    int rejectCount=0;
    pcl::PassThrough<pcl::PointXYZRGB> pass_;
    do
    {
        cloud_blob_prev = cloud_blob;
        cloud_blob = reader.getNextCloud();
        ros::Time ptime = cloud_blob->header.stamp;

        if (cloud_blob_prev != cloud_blob)
        {
            pcl::fromROSMsg(*cloud_blob, inp_cloud);
            //ROS_INFO ("PointCloud with %d data points and frame %s (%f) received.", (int)cloud.points.size (), cloud.header.frame_id.c_str (), cloud.header.stamp.toSec ());
            pcl_count++;
        }

        //    rosbag::View view(bag, rosbag::TopicQuery("/rgbdslam/my_clouds"));


        rosbag::View view_tf(bag, rosbag::TopicQuery("/tf"), ptime - ros::Duration(0, 1), ptime + ros::Duration(0, 100000000));
        //std::cerr<<(view_tf.size())<<endl;
        std::cerr << ptime << std::endl;
        //        std::cerr<<"qid:"<<pcl_ptr->header.seq<<endl;;
        tf_count = 0;

        tf::Transform final_tft;

        BOOST_FOREACH(rosbag::MessageInstance const mtf, view_tf)
        {
            tf::tfMessageConstPtr tf_ptr = mtf.instantiate<tf::tfMessage > ();
            assert(tf_ptr != NULL);
            std::vector<geometry_msgs::TransformStamped> bt;
            tf_ptr->get_transforms_vec(bt);
            tf::Transform tft(getQuaternion(bt[0].transform.rotation), getVector3(bt[0].transform.translation));

            //  transG.print();
            if (ptime == bt[0].header.stamp)
            {
                tf_count++;
                std::cerr << "tf qid:" << bt[0].header.seq << std::endl;
                final_tft = tft;
            }
            assert(tf_count <= 1);
        }

        if (tf_count == 1)
        {
            TransformG transG(final_tft);
            pcl::PointCloud<pcl::PointXYZRGB>::Ptr inp_cloud_ptr(new pcl::PointCloud<pcl::PointXYZRGB > (inp_cloud));
            //  transformPointCloud(transG.transformMat,inp_cloud_ptr,cloud_transformed);
            // applyFilters(inp_cloud_ptr,cloud_filtered);
            std::cerr << "Origin" << inp_cloud.sensor_origin_[0] << "," << inp_cloud.sensor_origin_[1] << "," << inp_cloud.sensor_origin_[2] << "," << inp_cloud.sensor_origin_[3] << std::endl;
            transG.print();

            pass_.setInputCloud(inp_cloud_ptr);
            pass_.filter(*cloud_filtered);
            appendNormals(cloud_filtered,cloud_normal);
            //app

            if (pcl_count == 1)
            {
                appendCamIndexAndDistance(cloud_normal,final_cloud,0,transG.getOrigin());
                transformsG.push_back(transG);
                pcl::PointCloud<PointT>::Ptr aCloud(new pcl::PointCloud<PointT > ());
                *aCloud=*cloud_normal;
                pointClouds.push_back(aCloud);
                pcl::KdTreeFLANN<PointT>::Ptr nnFinder(new pcl::KdTreeFLANN<PointT>);
                nnFinder->setInputCloud(aCloud);
                searchTrees.push_back(nnFinder);

            }
            else if (pcl_count % 5 == 1)
            {
                PointT cpoint;
                pcl::PointCloud<PointT>::Ptr aCloud(new pcl::PointCloud<PointT > ());
                *aCloud=*cloud_normal;
                pcl::KdTreeFLANN<PointT>::Ptr nnFinder(new pcl::KdTreeFLANN<PointT>);

                std::cerr<<" processing "<<pcl_count<<std::endl;
                for (unsigned int p = 0; p < cloud_normal->size(); p++)
                {
                    bool occluded=false;
                    cpoint = cloud_normal->points[p];
                    int c;
                        VectorG vpoint(cpoint.x,cpoint.y,cpoint.z);
                    if(!transG.isPointVisible(vpoint)) // if this point is not around the centre of it's own cam, ignore it
                        continue;
                    for (c = 0; c < transformsG.size(); c++)
                    {
                        TransformG ctrans=transformsG[c];
                        if((ctrans.isPointVisible(vpoint))) // is it already visibile in a prev camera? then it might be an outlier
                        {
                            // is it also not occluded in the same camera in which it is visible? only then it will be an outlier
                            pcl::PointCloud<PointT>::Ptr apc=pointClouds[c];
                            pcl::KdTreeFLANN<PointT>::Ptr annFinder=searchTrees[c];
                            int lpt;
                            // if any point in the same frame occludes it, it wont be an outlier
                                VectorG cam2point=vpoint.subtract(ctrans.getOrigin());
                                double distance=cam2point.getNorm();
                                double radiusCyl=0.05;
                                cam2point.normalize();
                                int numPointsInBw=(int)(distance/radiusCyl); // floor because we dont wanna consider points very closet to the target
                                set<int> indices;
                                indices.clear();
                            for(lpt=2;lpt<numPointsInBw-1;lpt++) // -1 because because we dont wanna consider points very closet to the target ... they could be false occulusions
                            {
                                VectorG offset=cam2point.multiply(lpt*radiusCyl);
                                VectorG linePt = offset.add(ctrans.getOrigin());
                                int numNeighbors = annFinder->radiusSearch(linePt.getAsPoint(), radiusCyl, k_indices, k_distances, 20);
                                //apc->
                                for (int nn = 0; nn < numNeighbors; nn++)
                                {
                                        indices.insert(k_indices[nn]);
                                }
                                k_indices.clear();
                                k_distances.clear();

                            }

                            set<int>::iterator iter;
                            occluded=false;
                            for (iter = indices.begin(); iter != indices.end(); iter++)
                            {
                                VectorG ppcPointV(apc->points[*iter]);
                                double distanceLine = ppcPointV.computeDistanceSqrFromLine(ctrans.getOrigin(), vpoint);
                                if (distanceLine < (0.004 * 0.004) && ppcPointV.isInsideLineSegment(ctrans.getOrigin(), vpoint))
                                {
                                    
                                    occluded = true;
                                    break;
                                }
                            }

                            if(!occluded)
                            {
                                break; // reject this point, must present in prev camera and not occluded
                            }
                            else
                            {
                                VectorG cam2point=vpoint.subtract(transG.getOrigin());//change to the cam of test point to check if this occluded point is a duplicate
                                double distance=cam2point.getNorm();
                                double radiusCylFalseOcc=0.1; // more than radiusCyl to ensure more repudiations.. if u change this, also change numPointsToConsider ... the idea is that for repudiation, we only wanna consider points close to the target
                                int numPointsToConsider=1;
                                cam2point.normalize();
                                int numPointsInBw=(int)ceil(distance/radiusCylFalseOcc); // ceil because we dont wanna miss points near the target
                                set<int> indices;
                                indices.clear();
                                int startIndex=numPointsInBw-numPointsToConsider;
                                if(startIndex<1)
                                    startIndex=1;
                                for (lpt = startIndex; lpt <= numPointsInBw; lpt++)
                                {
                                    VectorG offset = cam2point.multiply(lpt * radiusCylFalseOcc);
                                    VectorG linePt = offset.add(transG.getOrigin());// change origin to current cam
                                    int numNeighbors = annFinder->radiusSearch(linePt.getAsPoint(), radiusCylFalseOcc, k_indices, k_distances, 20);
                                    //apc->

                                    for (int nn = 0; nn < numNeighbors; nn++)
                                    {
                                        indices.insert(k_indices[nn]);
                                    }
                                    k_indices.clear();
                                    k_distances.clear();

                                }

                                set<int>::iterator iter;
                                occluded = false;
                                for (iter = indices.begin(); iter != indices.end(); iter++)
                                {
                                    VectorG ppcPointV(apc->points[*iter]);
                                    double distanceLine = ppcPointV.computeDistanceSqrFromLine(ctrans.getOrigin(), vpoint);
                                    if (distanceLine < (0.003 * 0.003*distance) && ppcPointV.isInsideLineSegment(ctrans.getOrigin(), vpoint)) //0.3 more the value, more repudiation => less points added
                                    {


                                        //occlusion possible

                                        if(cosNormal(apc->points[*iter],cpoint)>0.90) // more the value, less repudiation => more occlusion =>more points added
                                        {
                                            occluded = true;
                                            break; // =>point wont be added
                                        }
                                    }
                                }
                                //visible but occluded by some point in same frame
                                if(occluded)
                                {
                                    std::cerr<<"occlusion repudiation detected "<<numOcclRepudiated++ <<" pcl no:"<< pcl_count<<std::endl;
                                    break;
                                }
                                else
                                    std::cerr<<"occlusion detected "<<numOccluded++ <<" pcl no:"<< pcl_count<<std::endl;

                            }
                        }
                    }
                    if(c==transformsG.size())
                    {
                        pcl::PointXYGRGBCam newPoint;
                        newPoint.x=cpoint.x;
                        newPoint.y=cpoint.y;
                        newPoint.z=cpoint.z;
                        newPoint.rgb=cpoint.rgb;
                        newPoint.normal_x=cpoint.normal_x;
                        newPoint.normal_y=cpoint.normal_y;
                        newPoint.normal_z=cpoint.normal_z;
                        newPoint.cameraIndex=transformsG.size();
                        newPoint.distance=VectorG(cpoint).subtract(transG.getOrigin()).getNorm();
                        final_cloud->points.push_back(newPoint);
                    }
                    else
                    {
                        rejectCount++;
                    }
                }
                transformsG.push_back(transG);
                nnFinder->setInputCloud(aCloud);
                pointClouds.push_back(aCloud);
                searchTrees.push_back(nnFinder);
            writer.write<pcl::PointXYGRGBCam > ("/home/aa755/VisibilityMerged"+boost::lexical_cast<std::string>(pcl_count)+".pcd", *final_cloud, false);

            }
        }
        else
        {
            std::cerr << "no tf found";
        }



    }
    while (cloud_blob != cloud_blob_prev);

    std::cerr<<"nof rejected points "<<rejectCount;;
 //   applyFilters(final_cloud, cloud_filtered);

    bag.close();


    //  ros::NodeHandle n;
    //Instantiate the kinect image listener
    /*  OpenNIListener kinect_listener(n,
    //  OpenNIListener kinect_listener(
                                     "/rgbdslam/batch_clouds",
                                     "/rgbdslam/my_clouds"
                                     );//,  "/camera/depth/image_raw");
 
       ros::spin();
     */

    //  while(1){
    //   sleep(20);

    //  }

}



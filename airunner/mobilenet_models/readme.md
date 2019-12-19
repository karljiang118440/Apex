
#1、In  mobilenet functions there 

	  status = case_mobilenet("data/airunner/frozen_mobilenet_bn_qsym_final_part.pb",
                              "data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",
                              "data/airunner/test_classification.jpg",
                              "data/airunner/image_classification/imagenet_slim_labels.txt");
							  
							  
2 quanstions:

(1). The pb files used for what?
(2).if we can used a different pb files ?



#2.pb models:frozen_mobilenet_float_outputlayers_graph.pb



root@s32v234evb:~/karl# ./case_mobilenet_models.elf
Framebuffer mapped at 0x7f7d4ec000.
Too few or too many arguments
usage: ./case_mobilenet_models.elf(1/2/3/4/5/6/7/8/9)
1: mobilenet classification loop demo
2: mnetv1_ssd object detection loop demo.
3: mnetv2_ssd object detection loop demo.
4: mnetv2_ssdlite object detection loop demo.
5: mobilenet classification single image demo.
6: mnetv1_ssd object detection single image demo.
7: mnetv2_ssd object detection single image demo.
8: mnetv2_ssdlite object detection single image demo.
9: Exit the program.

Enter test index. 5
Running MobileNet graph for APEX
Net verification failed
Running MobileNet graph for CPU
Net verification failed
Error: Empty tensor.
mobilenet classification single image demo Failed to finish.


#3
status = case_mobilenet("data/airunner/mobilenet_v1_1.0_224_frozen.pb",
                              "data/airunner/mobilenet_v1_1.0_224_frozen.pb",
                              "data/airunner/test_classification.jpg",
                              "data/airunner/image_classification/imagenet_slim_labels.txt");


root@s32v234evb:~/karl# ./case_mobilenet_models.elf
Framebuffer mapped at 0x7f815d2000.
Too few or too many arguments
usage: ./case_mobilenet_models.elf(1/2/3/4/5/6/7/8/9)
1: mobilenet classification loop demo
2: mnetv1_ssd object detection loop demo.
3: mnetv2_ssd object detection loop demo.
4: mnetv2_ssdlite object detection loop demo.
5: mobilenet classification single image demo.
6: mnetv1_ssd object detection single image demo.
7: mnetv2_ssd object detection single image demo.
8: mnetv2_ssdlite object detection single image demo.
9: Exit the program.

Enter test index. 5
Running MobileNet graph for APEX
Too many inputs to merge
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_depthwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_depthwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_1_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_depthwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_8_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_13_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_13_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_0/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_1_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_4_pointwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_4_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_0/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_pointwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_0/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_8_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_1_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_pointwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_12_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_1_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_8_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_13_depthwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_4_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_12_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_12_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_depthwise/BatchNorm/gamma/read
Data dependency not met
Failed to load net
Running MobileNet graph for CPU
Too many inputs to merge
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_depthwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_depthwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_1_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_depthwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_8_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_13_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_13_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_0/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_1_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_4_pointwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_4_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_0/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_pointwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_0/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_8_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_1_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_pointwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_pointwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_12_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_1_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_7_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_9_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_8_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_11_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_depthwise/BatchNorm/moving_variance/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_13_depthwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_pointwise/weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_2_pointwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_6_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_4_depthwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_3_pointwise/BatchNorm/beta/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_12_depthwise/BatchNorm/moving_mean/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_12_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_pointwise/BatchNorm/gamma/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_5_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_depthwise/depthwise_weights/read
Skipping unsupported Identity layer MobilenetV1/Conv2d_10_depthwise/BatchNorm/gamma/read
Data dependency not met
Failed to load net
Error: Empty tensor.
mobilenet classification single image demo Failed to finish.




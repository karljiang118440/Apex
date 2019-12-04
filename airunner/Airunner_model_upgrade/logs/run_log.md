

#1、froezen_mssd.pb 

	run frozen_mssd.pb


Unvisited node
Unvisited node
Unvisited node
Unvisited node
Unvisited node
Unvisited node
Unvisited node
Unvisited node
Unvisited node
Unvisited node
Unvisited node
Skipping unsupported Exit layer Postprocessor/BatchMultiClassNonMaxSuppression/map/while/Exit_1
Skipping unsupported TensorArraySizeV3 layer Postprocessor/BatchMultiClassNonMaxSuppression/map/TensorArrayStack/TensorArraySizeV3
Skipping unsupported Range layer Postprocessor/BatchMultiClassNonMaxSuppression/map/TensorArrayStack/range
Skipping unsupported TensorArrayGatherV3 layer Postprocessor/BatchMultiClassNonMaxSuppression/map/TensorArrayStack/TensorArrayGatherV3
Skipping unsupported Identity layer detection_boxes
Time taken to Resize: 19657000 nanoseconds
Time taken to mssd object detection: 3417 nanoseconds
Net output box/class not found
Error: Empty APEX output tensor.
mnetv1_ssd object detection single image demo Failed to finish.


#2、./frozen_mssd_part_karl_1-pb.elf 

use quantization tools to quantiz frozen_mssd.pb

Enter test index. 6
Running MSSD graph for APEX
*** Error in `./frozen_mssd_part_karl_1-pb.elf': free(): invalid pointer: 0x0000000003ec5260 ***
Aborted



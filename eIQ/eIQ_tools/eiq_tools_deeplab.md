

# 4.2
Quantizing MobileNet(v1/v2) + SSD(Lite)
Note tensorflow/models commit: 5556579376519853b41b54227759d2ee9f7d042c on October 28, 2017 affects the
post-processing for all MobileNet + SSD variants. Models before this commit use version 1 post-processing, models
after use version 2.
This example uses MobileNetv1 + SSD, but the same steps have been verified for MobileNetv2 + SSD and MobileNetv2
+ SSDLite.
1. Install the TensorFlow Object Detection Framework

2. Train a MobileNet + SSD model and freeze it, or download pre-trained model from:
• MobileNetV1+SSD
• MobileNetV2+SSD
• MobileNetV2+SSDLite

3. Use TensorFlow graph transform_graph tool to remove pre/post processing part off mssd model
$bazel build tensorflow/tools/graph_transforms:transform_graph
$bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
--in_graph=$EIQ_AUTO_DATA/models/tf/mssdv1/deployment/ssd_mobilenet_v1_coco_2018_01_28/frozen_inference_graph.pb \
--out_graph=$EIQ_AUTO_DATA/models/tf/mssdv1/deployment/ssd_mobilenet_v1_coco_2018_01_28/frozen_inference_graph_ssd_part.pb \
--inputs=Preprocessor/sub --outputs=concat,concat_1 \
--transforms='strip_unused_nodes(type=float, shape="1,300,300,3")
remove_nodes(op=Identity,op=CheckNumerics) fold_constants(ignore_errors=true)'

4. Use TensorFlow optimizing compiler to prepare the model for inference
$
bazel run --config=opt //tensorflow/lite/toco:toco -- \
--input_file=$EIQ_AUTO_DATA/models/tf/mssdv1/deployment/ssd_mobilenet_v1_coco_2018_01_28/frozen_inference_graph_ssd_part.pb \
--output_file=$EIQ_AUTO_DATA/models/tf/mssdv1/deployment/ssd_mobilenet_v1_coco_2018_01_28/frozen_mobilenetv1_ssd_float.pb \
--input_format=TENSORFLOW_GRAPHDEF --output_format=TENSORFLOW_GRAPHDEF --input_shape=1,300,300,3 \
--input_array=Preprocessor/sub --output_arrays=concat,concat_1 --drop_control_dependency

5. Quantize the core model, without pre/post processing
$cd $EIQ_AUTO_DATA/models/tf/mssdv1/workspace
$python quantize_graph.py



6. Optional Verify the optimized model

$cd $EIQ_AUTO_DATA/models/tf/mssdv1/workspace
$python graph_runner.py

[0.03908283 0.01921482 0.87210363 0.31577367] 0.94068974 18.0
[0.10951498 0.40283597 0.9246464  0.9730481 ] 0.93450415 18.0




4.4
Quantizing Deeplab
1. Download pre-trained model (mobilenetv2_dm05_coco_voc_trainaug) from:
• Deeplab

2. Optional Verify the model
$cd $EIQ_AUTO_DATA/models/tf/deeplab/workspace
$python demo.py

3. Quantize the model by passing one sample image through the network
$cd $EIQ_AUTO_DATA/models/tf/deeplab/workspace
$python quantize_graph.py

4. Use TensorFlow graph transform_graph tool to remove pre/post processing part of deeplab model
$bazel build tensorflow/tools/graph_transforms:transform_graph
$bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
--in_graph=$EIQ_AUTO_DATA/models/tf/deeplab/deployment/deeplab_quant.pb \
--out_graph=$EIQ_AUTO_DATA/models/tf/deeplab/deployment/deeplab_core_quant_2.pb \
--inputs=MobilenetV2/MobilenetV2/input --outputs=ResizeBilinear_2 \
--transforms='strip_unused_nodes(type=float, shape="1,513,513,3")'
The output: deeplab_core_quant_2.pb is the model to be used on EVB/APEX.


#4.1Quantizing MobileNet (v1/v2)

1. Train a MobileNet model and freeze it, or download pre-trained model from
• MobileNetV1
• MobileNetV2

2. Use TensorFlow Optimizing compiler to prepare the model for inference
This command needs to be run from the ’tensorflow’ folder

export EIQ_AUTO_DATA=/media/jcq/Work/NXP/S32V2-eIQAuto01_RTM_2_0_0/s32v234_sdk/tools/eiq_auto_data

$bazel run --config=opt //tensorflow/lite/toco:toco -- \
--drop_control_depenency \
--input_file=$EIQ_AUTO_DATA/models/tf/mobilenetv1/deployment/mobilenet_v1_1.0_224_frozen.pb \
--output_file=$EIQ_AUTO_DATA/models/tf/mobilenetv1/deployment/mobilenet_v1_1.0_224_frozen_float.pb \
--input_format=TENSORFLOW_GRAPHDEF \
--output_format=TENSORFLOW_GRAPHDEF \
--input_shape=1,224,224,3 \
--input_array=input \
--output_array=MobilenetV[1/2]/Predictions/Reshape_1

3. Optional Verify the optimized model
$cd $EIQ_AUTO_DATA/models/tf/mobilenetv1/workspace
$python graph_runner.py

tiger shark 0.45346787571907043

4. Quantize full model
$cd $EIQ_AUTO_DATA/models/tf/mobilenetv1/workspace
$python quantize_graph.py


5. Extract quantized feature extractor to be run in fixed point to achieve high performance
This command needs to be run from the ’tensorflow’ folder
$bazel build tensorflow/tools/graph_transforms:transform_graph
$bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
--in_graph=$EIQ_AUTO_DATA/models/tf/mobilenetv1/deployment/frozen_mb1_dms_quant.pb \
--out_graph=$EIQ_AUTO_DATA/models/tf/mobilenetv1/deployment/frozen_mb1_dms_quant_part.pb \
--inputs=input --outputs=MobilenetV1/MobilenetV1/Conv2d_13_pointwise/Relu6 \
--transforms='strip_unused_nodes(type=float, shape="1,224,224,3")'


6. Extract classification head (last 3 layers) to be run in floating point to achieve better accuracy
This command needs to be run from the ’tensorflow’ folder
$bazel build tensorflow/tools/graph_transforms:transform_graph
$bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
--in_graph=$EIQ_AUTO_DATA/models/tf/mobilenetv1/deployment/frozen_mb1_dms_float.pb \
--out_graph=$EIQ_AUTO_DATA/models/tf/mobilenetv1/deployment/frozen_mb1_dms_float_part.pb \
--inputs=MobilenetV1/MobilenetV1/Conv2d_13_pointwise/Relu6 \
--outputs=MobilenetV1/Predictions/Reshape_1 \
--transforms='strip_unused_nodes(type=float, shape="1,7,7,256")'

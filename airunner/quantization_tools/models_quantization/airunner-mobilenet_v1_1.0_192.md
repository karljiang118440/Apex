#path set
export MODEL_DIR=/home/jcq/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/s32v234_sdk/libs/dnn/airunner/offline/quantization_tools/model/mobilenet/mobilenet_v1_1.0_192_frozen
export DATA_DIR=/home/jcq/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/s32v234_sdk/libs/dnn/airunner/offline/quantization_tools/data



#对 mobilenet 进行的量化压缩



python mnet_imagelabelling_test.py \
--image=$DATA_DIR/grace_hopper.jpg  \
--graph=$MODEL_DIR/mobilenet_v1_1.0_192_frozen.pb  \
--input_height=192 --input_width=192

>

military uniform 0.95247304
cornet 0.010702857
pickelhaube 0.0067657586
suit 0.0054707765
bearskin 0.003906955






bazel run --config=opt //tensorflow/contrib/lite/toco:toco -- \
 --drop_control_depenency \
 --input_file=$MODEL_DIR/mobilenet_v1_1.0_192_frozen.pb \
 --output_file=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn.pb \
 --input_format=TENSORFLOW_GRAPHDEF --output_format=TENSORFLOW_GRAPHDEF \
 --input_shape=1,192,192,3 --input_array=input --output_array=MobilenetV1/Predictions/Reshape_1



python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn.pb \
 --input_height=192 --input_width=192


>
military uniform 0.9524729
cornet 0.010702916
pickelhaube 0.0067657707
suit 0.005470807
bearskin 0.003906962







 
python quantize_graph_mnet.py \
 --input=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn.pb \
 --output_node_names=MobilenetV1/Predictions/Reshape_1 \
 --print_nodes --output=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn_qsym.pb \
 --mode=weights_sym –logtostderr
 


python mnet_minmax_freeze.py \
 --in_graph=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn_qsym.pb \
 --out_graph=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn_qsym_final.pb \
 --out_graph_part=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn_qsym_final_part.pb \
 --output_layer=MobilenetV1/Predictions/Reshape_1 \
 --output_layer_part=MobilenetV1/MobilenetV1/Conv2d_13_pointwise/Relu6




python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn_qsym.pb \
 --input_height=192 --input_width=192


>

military uniform 0.96776396
pickelhaube 0.0058476147
cornet 0.0046731825
bow tie 0.0037774774
bulletproof vest 0.003768446










bazel run --config=opt //tensorflow/contrib/lite/toco:toco --\
 --drop_control_depenency\
 --input_file=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn.pb\
 --output_file=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_float_outputlayers_graph.pb\
 -input_format=TENSORFLOW_GRAPHDEF\
 --output_format=TENSORFLOW_GRAPHDEF\
 --input_shape=1,7,7,256\
 --input_array=MobilenetV1/MobilenetV1/Conv2d_13_pointwise/Relu6\
 --output_arrays=MobilenetV1/Predictions/Reshape_1
 
 
 python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v1_1.0_192_frozen_bn_qsym_final.pb \
 --input_height=192 --input_width=192
 
 >
 
 military uniform 0.96776396
pickelhaube 0.0058476147
cornet 0.0046731825
bow tie 0.0037774774
bulletproof vest 0.003768446
 
 
 
 
 
 


export DATA_DIR=/home/jcq/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/s32v234_sdk/libs/dnn/airunner/offline/quantization_tools/data
export MODEL_DIR=/home/jcq/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/s32v234_sdk/libs/dnn/airunner/offline/quantization_tools/model/mobilenetv2/mobilenet_v2_1.0_224


#一、对 mobilenet 进行的量化压缩


3. batch normalization

bazel run --config=opt //tensorflow/contrib/lite/toco:toco -- \
 --drop_control_depenency \
 --input_file=$MODEL_DIR/mobilenet_v2_1.0_224_frozen.pb \
 --output_file=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn.pb \
 --input_format=TENSORFLOW_GRAPHDEF --output_format=TENSORFLOW_GRAPHDEF \
 --input_shape=1,224,224,3 \
 --input_array=input \
 --output_array=MobilenetV2/Predictions/Reshape_1

python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen.pb \
 --input_height=224 --input_width=224
 
 
4. quantization 
 python quantize_graph_mnet.py \
 --input=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn.pb \
--output_node_names=MobilenetV2/Predictions/Reshape_1 \
 --print_nodes --output=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym.pb \
 --mode=weights_sym –logtostderr

python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym.pb \
 --input_height=224 --input_width=224


5. freeze quantization model (min/max range)

python mnet_minmax_freeze.py \
--in_graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym.pb \
--out_graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym_final.pb \
--out_graph_part=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym_final_part.pb \
--output_layer=MobilenetV2/Predictions/Reshape_1 \
--output_layer_part=MobilenetV2/Conv_1/Relu6




python mnet_imagelabelling_test.py \
--image=$DATA_DIR/grace_hopper.jpg  \
--graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym_final.pb  \
--input_height=224 --input_width=224

7. extract sub-model

bazel run --config=opt //tensorflow/contrib/lite/toco:toco -- \
--drop_control_depenency \
--input_file=$MODEL_DIR/mobilenet/mobilenet_v2_1.0_224_frozen_bn.pb \
--output_file=$MODEL_DIR/mobilenet/mobilenet_v1_1.0_224_float_outputlayers_graph.pb \
-input_format=TENSORFLOW_GRAPHDEF --output_format=TENSORFLOW_GRAPHDEF \
--input_shape=1,7,7,256 \
--input_array=MobilenetV1/MobilenetV1/Conv2d_13_pointwise/Relu6 \
--output_arrays=MobilenetV1/Predictions/Reshape_1





#二、添加其他的压缩量化方法



export PYTHONPATH=$PYTHONPATH:/home/jcq/tensorflow/models-master/research/slim
export PYTHONPATH=$PYTHONPATH:/home/jcq/tensorflow/models-master/research/object_detection

##2.1 、针对 inception_v3_2016_08_28_frozen.pb 
 
bazel build tensorflow/tools/graph_transforms:transform_graph
bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
  --in_graph=$MODEL_DIR/inception_v3_2016_08_28_frozen.pb/inception_v3_2016_08_28_frozen.pb \
  --out_graph=$MODEL_DIR/inception_v3_2016_08_28_frozen.pb/quantized_graph.pb \
  --inputs=input \
  --outputs=InceptionV3/Predictions/Reshape_1 \
  --transforms='add_default_attributes strip_unused_nodes(type=float, shape="1,299,299,3")
    remove_nodes(op=Identity, op=CheckNumerics) fold_constants(ignore_errors=true)
    fold_batch_norms fold_old_batch_norms quantize_weights quantize_nodes
    strip_unused_nodes sort_by_execution_order'

	确实将原先的模型压缩为原来的1/4



##2.2 、针对 mobilenet_v2_1.0_224_frozen.pb 
 
bazel build tensorflow/tools/graph_transforms:transform_graph
bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
  --in_graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen.pb \
  --out_graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_quantization.pb \
  --inputs=input \
  --outputs=MobilenetV1/Predictions/Reshape_1 \
  --transforms='add_default_attributes strip_unused_nodes(type=float, shape="1,224,224,3")
    remove_nodes(op=Identity, op=CheckNumerics) fold_constants(ignore_errors=true)
    fold_batch_norms fold_old_batch_norms quantize_weights quantize_nodes
    strip_unused_nodes sort_by_execution_order'


# 运行结果：

	4.5M mobilenet_v2_1.0_224_frozen_quantization.pb


python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_quantization.pb \
 --input_height=224 --input_width=224

military uniform 0.8151586
bow tie 0.121474616
Windsor tie 0.031967007
bulletproof vest 0.006393401
cornet 0.0031967005

PC 端的运行结果毫无问题，但是并无时间上的显示

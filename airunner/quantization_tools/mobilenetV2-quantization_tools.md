export DATA_DIR=/home/karljiang/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/s32v234_sdk/libs/dnn/airunner/offline/quantization_tools/data
export DATA_DIR=/home/karljiang/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/s32v234_sdk/libs/dnn/airunner/offline/quantization_tools/MODEL

#一、对 mobilenet 进行的量化压缩






bazel run --config=opt //tensorflow/contrib/lite/toco:toco -- \
 --drop_control_depenency \
 --input_file=$MODEL_DIR/mobilenet_v2_1.0_224_frozen.pb \
 --output_file=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn.pb \
 --input_format=TENSORFLOW_GRAPHDEF --output_format=TENSORFLOW_GRAPHDEF \
 --input_shape=1,224,224,3 --input_array=input --output_array=MobilenetV2/Predictions/Reshape_1


python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn.pb \
 --input_height=224 --input_width=224
 
 
 python quantize_graph_mnet.py \
 --input=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn.pb \
--output_node_names=MobilenetV1/Predictions/Reshape_1 \
 --print_nodes --output=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym.pb \
 --mode=weights_sym –logtostderr


python mnet_minmax_freeze.py \
--in_graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym.pb \
--out_graph=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym_final.pb \
--out_graph_part=$MODEL_DIR/mobilenet_v2_1.0_224_frozen_bn_qsym_final_part.pb \
--output_layer=MobilenetV1/Predictions/Reshape_1 \
--output_layer_part=MobilenetV1/MobilenetV1/Conv2d_13_pointwise/Relu6




python mnet_imagelabelling_test.py \
--image=$DATA_DIR/grace_hopper.jpg  \
--graph=$MODEL_DIR/frozen_mobilenet.pb  \
--input_height=224 --input_width=224



export PYTHONPATH=$PYTHONPATH:/home/karljiang/TensorFlow/tensorflow-1.9.0-rc1/tensorflow/models/reserch:/home/karljiang/TensorFlow/tensorflow-1.9.0-rc1/tensorflow/models/reserch/slim

export PYTHONPATH=$PYTHONPATH:/home/karljiang/TensorFlow/tensorflow-1.9.0-rc1/tensorflow/models/reserch/slim

export PYTHONPATH=$PYTHONPATH:/home/karljiang/TensorFlow/tensorflow-1.9.0-rc1/tensorflow/models/reserch/object_detection



#三、添加其他的压缩量化方法


bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
--in_graph=$MODEL_DIR/frozen_mssd.pb \
--out_graph=$MODEL_DIR/frozen_mssd_part.pb \
--inputs=Preprocessor/sub --outputs=concat,concat_1 \
--transforms='weights strip_unused_nodes(type=float, shape="1,300,300,3") remove_nodes(op=Identity, op=CheckNumerics) fold_constants(ignore_errors=true)'

##3.1、权重量化

bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
--in_graph=$MODEL_DIR/frozen_mssd.pb \
--out_graph=$MODEL_DIR/frozen_mssd_part_karl.pb \
--inputs=Preprocessor/sub --outputs=concat,concat_1 \
--transforms='quantize_weights strip_unused_nodes(type=float, shape="1,300,300,3") remove_nodes(op=Identity, op=CheckNumerics) fold_constants(ignore_errors=true)'


>:并没有对模型压缩太多，还是27M，相当于没有改变


##3.2、添加所有选项

参考 https://blog.csdn.net/cokeonly/article/details/79024279 

bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
  --in_graph=tensorflow/examples/label_image/data/inception_v3_2016_08_28_frozen.pb \
  --out_graph=/tmp/quantized_graph.pb \
  --inputs=input \
  --outputs=InceptionV3/Predictions/Reshape_1 \
  --transforms='add_default_attributes strip_unused_nodes(type=float, shape="1,299,299,3")
    remove_nodes(op=Identity, op=CheckNumerics) fold_constants(ignore_errors=true)
    fold_batch_norms fold_old_batch_norms quantize_weights quantize_nodes
    strip_unused_nodes sort_by_execution_order'
	

bazel-bin/tensorflow/tools/graph_transforms/transform_graph \
--in_graph=$MODEL_DIR/frozen_mssd.pb \
--out_graph=$MODEL_DIR/frozen_mssd_part_karl_1.pb \
--inputs=Preprocessor/sub --outputs=concat,concat_1 \
--transforms='add_default_attributes strip_unused_nodes(type=float, shape="1,300,300,3")
    remove_nodes(op=Identity, op=CheckNumerics) fold_constants(ignore_errors=true)
    fold_batch_norms fold_old_batch_norms quantize_weights quantize_nodes
    strip_unused_nodes sort_by_execution_order'	
	

>:对应的模型减小为原来1/4 大小，非常有效，在这个基础上继续进行压缩量化看运行效果。


bazel run --config=opt //tensorflow/contrib/lite/toco:toco \
-- --input_file=$MODEL_DIR/frozen_mssd_part_karl_1.pb \
--output_file=$MODEL_DIR/frozen_mssd_part_karl_1_bn.pb \
--input_format=TENSORFLOW_GRAPHDEF \
--output_format=TENSORFLOW_GRAPHDEF \
--input_shape=1,300,300,3 \
--input_array=Preprocessor/sub --output_arrays=concat,concat_1 --drop_control_dependency



python quantize_graph_mnet.py \
--input=$MODEL_DIR/frozen_mssd_part_karl_1_bn.pb \
--output=$MODEL_DIR/frozen_mssd_part_karl_1_bn_quant.pb \
--output_node_names=concat,concat_1 --mode=weights_sym_assign --print_nodes








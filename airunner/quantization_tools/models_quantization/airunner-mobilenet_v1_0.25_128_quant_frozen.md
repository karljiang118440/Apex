#path set
export MODEL_DIR=/home/jcq/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/s32v234_sdk/libs/dnn/airunner/offline/quantization_tools/model/mobilenet/mobilenet_v1_0.25_128_quant
export DATA_DIR=/home/jcq/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/SW32V23-VSDK-AIRUNNER-CODE_DROP-1.2.0-2/s32v234_sdk/libs/dnn/airunner/offline/quantization_tools/data



#对 mobilenet 进行的量化压缩




bazel run --config=opt //tensorflow/contrib/lite/toco:toco -- \
 --drop_control_depenency \
 --input_file=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen.pb \
 --output_file=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn.pb \
 --input_format=TENSORFLOW_GRAPHDEF --output_format=TENSORFLOW_GRAPHDEF \
 --input_shape=1,128,128,3 --input_array=input --output_array=MobilenetV1/Predictions/Reshape_1


python mnet_imagelabelling_test.py \
--image=$DATA_DIR/grace_hopper.jpg  \
--graph=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen.pb  \
--input_height=128 --input_width=128
>
academic gown 0.22379452
suit 0.1511436
mortarboard 0.089559
bow tie 0.089559
sombrero 0.035840157



python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn.pb \
 --input_height=128 --input_width=128
 
 
 >
academic gown 0.22821845
suit 0.11864549
mortarboard 0.070302635
sombrero 0.06168104
bow tie 0.06168104
 
 
 
python quantize_graph_mnet.py \
 --input=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn.pb \
 --output_node_names=MobilenetV1/Predictions/Reshape_1 \
 --print_nodes --output=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn_qsym.pb \
 --mode=weights_sym –logtostderr
 
python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn_qsym.pb \
 --input_height=128 --input_width=128
>

academic gown 0.17400584
suit 0.10310605
sombrero 0.0793678
mortarboard 0.069634505
bow tie 0.069634505


 


python mnet_minmax_freeze.py \
 --in_graph=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn_qsym.pb \
 --out_graph=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn_qsym_final.pb \
 --out_graph_part=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn_qsym_final_part.pb \
 --output_layer=MobilenetV1/Predictions/Reshape_1 \
 --output_layer_part=MobilenetV1/MobilenetV1/Conv2d_13_pointwise/Relu6



MobilenetV1/Logits/Conv2d_1c_1x1/BiasAdd/min_log
MobilenetV1/Logits/Conv2d_1c_1x1/BiasAdd/max_log
Traceback (most recent call last):
  File "mnet_minmax_freeze.py", line 382, in <module>
    {input_operation.outputs[0]: t})
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/client/session.py", line 900, in run
    run_metadata_ptr)
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/client/session.py", line 1135, in _run
    feed_dict_tensor, options, run_metadata)
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/client/session.py", line 1316, in _do_run
    run_metadata)
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/client/session.py", line 1335, in _do_call
    raise type(e)(node_def, op, message)
tensorflow.python.framework.errors_impl.InvalidArgumentError: Input to reshape is a tensor with 4004 values, but the requested shape has 1001
	 [[Node: import/MobilenetV1/Logits/SpatialSqueeze = Reshape[T=DT_FLOAT, Tshape=DT_INT32, _device="/job:localhost/replica:0/task:0/device:CPU:0"](import/MobilenetV1/Logits/Conv2d_1c_1x1/act_quant/FakeQuantWithMinMaxVars, import/MobilenetV1/Logits/SpatialSqueeze_shape)]]

Caused by op u'import/MobilenetV1/Logits/SpatialSqueeze', defined at:
  File "mnet_minmax_freeze.py", line 322, in <module>
    graph, graph_def = load_graph(model_file)
  File "mnet_minmax_freeze.py", line 112, in load_graph
    tf.import_graph_def(graph_def)
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/util/deprecation.py", line 432, in new_func
    return func(*args, **kwargs)
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/framework/importer.py", line 442, in import_graph_def
    _ProcessNewOps(graph)
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/framework/importer.py", line 234, in _ProcessNewOps
    for new_op in graph._add_new_tf_operations(compute_devices=False):  # pylint: disable=protected-access
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/framework/ops.py", line 3563, in _add_new_tf_operations
    for c_op in c_api_util.new_tf_operations(self)
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/framework/ops.py", line 3450, in _create_op_from_tf_operation
    ret = Operation(c_op, self)
  File "/home/jcq/.local/lib/python2.7/site-packages/tensorflow/python/framework/ops.py", line 1740, in __init__
    self._traceback = self._graph._extract_stack()  # pylint: disable=protected-access

InvalidArgumentError (see above for traceback): Input to reshape is a tensor with 4004 values, but the requested shape has 1001
	 [[Node: import/MobilenetV1/Logits/SpatialSqueeze = Reshape[T=DT_FLOAT, Tshape=DT_INT32, _device="/job:localhost/replica:0/task:0/device:CPU:0"](import/MobilenetV1/Logits/Conv2d_1c_1x1/act_quant/FakeQuantWithMinMaxVars, import/MobilenetV1/Logits/SpatialSqueeze_shape)]]











python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn_qsym_final.pb \
 --input_height=224 --input_width=224
 
 
 python mnet_imagelabelling_test.py \
 --image=$DATA_DIR/grace_hopper.jpg \
 --graph=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_float_outputlayers_graph.pb \
 --input_height=224 --input_width=224

bazel run --config=opt //tensorflow/contrib/lite/toco:toco --\
 --drop_control_depenency\
 --input_file=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_bn.pb\
 --output_file=$MODEL_DIR/mobilenet_v1_0.25_128_quant_frozen_float_outputlayers_graph.pb\
 -input_format=TENSORFLOW_GRAPHDEF\
 --output_format=TENSORFLOW_GRAPHDEF\
 --input_shape=1,7,7,256\
 --input_array=MobilenetV1/MobilenetV1/Conv2d_13_pointwise/Relu6\
 --output_arrays=MobilenetV1/Predictions/Reshape_1
import tensorflow as tf 
import tf2onnx
from tf2onnx import loader

# graph
conv1_w = tf.Variable(tf.random_normal([3, 3, 2, 3]))
conv1_b = tf.Variable(tf.random_normal([3]))
conv2_w = tf.Variable(tf.random_normal([3, 3, 3, 1]))
conv2_b = tf.Variable(tf.random_normal([1]))
xs = tf.placeholder(tf.float32, shape=[1, 12, 12, 2], name="input")
conv1 = tf.nn.conv2d(xs, conv1_w, strides=[1,1,1,1], padding='SAME') + conv1_b
conv2 = tf.nn.conv2d(conv1, conv2_w, strides=[1,1,1,1], padding='SAME') + conv2_b
tf.identity(conv2, name='output')
# get output_graph_def
init = tf.global_variables_initializer()
with tf.Session() as sess:
    sess.run(init)
    output_graph_def = loader.freeze_session(sess, output_names=["output:0"])
# to onnx
tf.reset_default_graph()
with tf.Graph().as_default() as tf_graph:
    tf.import_graph_def(output_graph_def, name='')
    onnx_graph = tf2onnx.tfonnx.process_tf_graph(tf_graph, input_names=["input:0"], output_names=["output:0"], opset=11)
    model_proto = onnx_graph.make_model("test")
    with open("./model/tfmodel.onnx", "wb") as f:
        f.write(model_proto.SerializeToString())
'''
# show
import onnx 
import netron
print(onnx.load('./model/tfmodel.onnx'))
netron.start('./model/tfmodel.onnx')
'''        

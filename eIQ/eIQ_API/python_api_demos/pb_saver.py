#!/usr/bin/python
# -*- coding:utf-8 -*-

import tensorflow as tf
from tensorflow.python.framework import graph_util
 
v1 = tf.Variable(tf.constant(1.0, shape=[1]), name="v1")
v2 = tf.Variable(tf.constant(2.0, shape=[1]), name="v2")
result = v1 + v2
 
with tf.Session() as sess:
    sess.run(tf.global_variables_initializer())
    # 导出当前计算图的GraphDef部分，即从输入层到输出层的计算过程部分
    graph_def = tf.get_default_graph().as_graph_def()
    output_graph_def = graph_util.convert_variables_to_constants(sess,
                                                        graph_def, ['add'])
 
    with tf.gfile.GFile("./model/test_model.pb", 'wb') as f:
        f.write(output_graph_def.SerializeToString())

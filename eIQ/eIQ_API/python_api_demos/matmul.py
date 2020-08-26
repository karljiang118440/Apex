#!/usr/bin/python
# -*- coding:utf-8 -*-
'''
一 反卷积实例
'''

import tensorflow as tf
import keras
config = tf.ConfigProto()
config.gpu_options.allow_growth = True
keras.backend.tensorflow_backend.set_session(tf.Session(config=config))


import tensorflow as tf
import numpy as np

a = tf.constant([[1,2],[3,4]])
b = tf.constant([[0,0],[1,0]])
c =a *b
d = tf.matmul(a,b)
with tf.Session() as sess:
	print(sess.run(d))
	print(sess.run(c))
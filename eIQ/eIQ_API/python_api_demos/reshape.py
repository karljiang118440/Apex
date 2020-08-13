#!/usr/bin/python
# -*- coding:utf-8 -*-
'''
一 反卷积实例
'''
'''
import tensorflow as tf
import keras
config = tf.ConfigProto()
config.gpu_options.allow_growth = True
keras.backend.tensorflow_backend.set_session(tf.Session(config=config))
'''

import tensorflow as tf
import numpy as np

#a_array=tf.constant[1,2,3,4,5,6]

input_data=[1,1,1,1,1,1,1,1,1]
#print(a_array)
print(input_data)
#print(tf.reshape(input_data,[-1,3]))
#print(tf.reshape(a_array,[2,3]))


image=np.array([[[1,2,3], [4,5,6]], [[1,1,1], [1,1,1]]])
print(image.shape)
a = image.reshape((-1,6))
print(a)


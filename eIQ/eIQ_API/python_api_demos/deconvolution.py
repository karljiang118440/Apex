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

#模拟数据
img = tf.Variable(tf.constant(1.0,shape=[1,4,4,1]))

kernel =tf.Variable(tf.constant([1.0,0,-1,-2],shape=[2,2,1,1]))

#分别进行VALID和SAME操作
conv =  tf.nn.conv2d(img,kernel,strides=[1,2,2,1],padding='VALID')
cons =  tf.nn.conv2d(img,kernel,strides=[1,2,2,1],padding='SAME')

#VALID填充计算方式 (n - f + 1)/s向上取整
print(conv.shape)
#SAME填充计算方式 n/s向上取整
print(cons.shape)

#在进行反卷积操作
contv = tf.nn.conv2d_transpose(conv,kernel,[1,4,4,1],strides=[1,2,2,1],padding='VALID')
conts = tf.nn.conv2d_transpose(cons,kernel,[1,4,4,1],strides=[1,2,2,1],padding='SAME')

with tf.Session() as sess:
    sess.run(tf.global_variables_initializer())
    
    print('kernel:\n',sess.run(kernel))
    print('conv:\n',sess.run(conv))
    print('cons:\n',sess.run(cons))
    print('contv:\n',sess.run(contv))
    print('conts:\n',sess.run(conts))
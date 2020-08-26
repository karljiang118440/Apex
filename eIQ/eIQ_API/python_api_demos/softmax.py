#!/usr/bin/python
# -*- coding:utf-8 -*-

import tensorflow as tf

s=tf.constant([1,1,2,3,4,5,10],dtype=tf.float32)
 
sm=tf.nn.softmax(s)
 
with tf.Session()as sess:

 print(sess.run(sm))
 print(sess.run(tf.argmax(sm)))


 '''
 [1.2210199e-04 1.2210199e-04 3.3190762e-04 9.0221845e-04 2.4524841e-03
 6.6665425e-03 9.8940265e-01]
6


 '''
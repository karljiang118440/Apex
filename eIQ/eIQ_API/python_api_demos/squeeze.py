#!/usr/bin/python
# -*- coding:utf-8 -*-

import tensorflow as tf

s=tf.constant([1,1,2,3,4,5,10],dtype=tf.float32)

t = tf.constant([1, 2, 1, 3, 1, 1],dtype = tf.float32)
 
sm=tf.nn.softmax(s)
t_squeeze = tf.squeeze(t)
 
with tf.Session()as sess:

 print(sess.run(sm))
 print(sess.run(tf.argmax(sm)))
 print(tf.shape(tf.squeeze(t)))
 print(sess.run(t_squeeze))





 '''

 't' is a tensor of shape [1, 2, 1, 3, 1, 1]
  tf.shape(tf.squeeze(t))  # [2, 3]


  Or, to remove specific size 1 dimensions:


  # 't' is a tensor of shape [1, 2, 1, 3, 1, 1]
  tf.shape(tf.squeeze(t, [2, 4]))  # [1, 2, 3, 1]




 [1.2210199e-04 1.2210199e-04 3.3190762e-04 9.0221845e-04 2.4524841e-03
 6.6665425e-03 9.8940265e-01]
6


 '''
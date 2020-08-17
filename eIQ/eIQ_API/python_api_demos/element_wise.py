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

import numpy as np

# 2-D array: 2 x 3
two_dim_matrix_one = np.array([[1, 2, 3], [4, 5, 6]])
another_two_dim_matrix_one = np.array([[7, 8, 9], [4, 7, 1]])

# 对应元素相乘 element-wise product
element_wise = two_dim_matrix_one * another_two_dim_matrix_one
print('element wise product: %s' %(element_wise))

# 对应元素相乘 element-wise product
element_wise_2 = np.multiply(two_dim_matrix_one, another_two_dim_matrix_one)
print('element wise product: %s' % (element_wise_2))







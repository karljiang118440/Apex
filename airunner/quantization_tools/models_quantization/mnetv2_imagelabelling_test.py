# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

#******************************************************************************
#
# NXP Confidential Proprietary
#
# Copyright 2017 NXP
# NOT A CONTRIBUTION
# All Rights Reserved
#
#******************************************************************************
#
# THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
#*****************************************************************************/

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import sys

import numpy as np
import tensorflow as tf

test_node_module = tf.load_op_library('./kernels/minmax_node.so')

_R_MEAN = 123.68
_G_MEAN = 116.78
_B_MEAN = 103.94

def load_graph(model_file):
  graph = tf.Graph()
  graph_def = tf.GraphDef()

  with open(model_file, "rb") as f:
    graph_def.ParseFromString(f.read())
  with graph.as_default():
    tf.import_graph_def(graph_def)

  return graph

def read_tensor_from_image_file(file_name, input_height=299, input_width=299,
				input_mean=0, input_std=255):
  input_name = "file_reader"
  output_name = "normalized"
  file_reader = tf.read_file(file_name, input_name)
  if file_name.endswith(".png"):
    image_reader = tf.image.decode_png(file_reader, channels = 3,
                                       name='png_reader')
  elif file_name.endswith(".gif"):
    image_reader = tf.squeeze(tf.image.decode_gif(file_reader,
                                                  name='gif_reader'))
  elif file_name.endswith(".bmp"):
    image_reader = tf.image.decode_bmp(file_reader, name='bmp_reader')
  else:
    image_reader = tf.image.decode_jpeg(file_reader, channels = 3,
                                        name='jpeg_reader')
  float_caster = tf.cast(image_reader, tf.float32)
  dims_expander = tf.expand_dims(float_caster, 0);
  resized = tf.image.resize_bilinear(dims_expander, [input_height, input_width])
  normalized = tf.divide(tf.subtract(resized, [input_mean]), [input_std])
  sess = tf.Session()
  result = sess.run(normalized)

  return result

def _crop(image, offset_height, offset_width, crop_height, crop_width):
  original_shape = tf.shape(image)
  cropped_shape = tf.stack([crop_height, crop_width, original_shape[2]])
  offsets = tf.to_int32(tf.stack([offset_height, offset_width, 0]))

  image = tf.slice(image, offsets, cropped_shape)
  return tf.reshape(image, cropped_shape)  



def _central_crop(image, crop_height, crop_width):
  """Performs central crops of the given image list.
  Args:
    image_list: a list of image tensors of the same dimension but possibly
      varying channel.
    crop_height: the height of the image following the crop.
    crop_width: the width of the image following the crop.
  Returns:
    the list of cropped images.
  """
  image_height = tf.shape(image)[0]
  image_width = tf.shape(image)[1]

  offset_height = (image_height - crop_height) / 2
  offset_width = (image_width - crop_width) / 2

  output = _crop(image, offset_height, offset_width,
                         crop_height, crop_width)
  return output

def _mean_image_subtraction(image, means):
  num_channels = image.get_shape().as_list()[-1]
  
  channels = tf.split(axis=2, num_or_size_splits=num_channels, value=image)
  for i in range(num_channels):
    channels[i] -= means[i]
  return tf.concat(axis=2, values=channels)


 

def read_tensor_from_image_file_vgg(file_name, output_height=224, output_width=224,
                                input_mean=0, input_std=255):
  input_name = "file_reader"
  output_name = "normalized"
  file_reader = tf.read_file(file_name, input_name)
  if file_name.endswith(".png"):
    image_reader = tf.image.decode_png(file_reader, channels = 3,
                                       name='png_reader')
  elif file_name.endswith(".gif"):
    image_reader = tf.squeeze(tf.image.decode_gif(file_reader,
                                                  name='gif_reader'))
  elif file_name.endswith(".bmp"):
    image_reader = tf.image.decode_bmp(file_reader, name='bmp_reader')
  else:
    image_reader = tf.image.decode_jpeg(file_reader, channels = 3,
                                        name='jpeg_reader')

  shape = tf.shape(image_reader) 

  height = shape[0]
  width = shape[1]
  smallest_side = 256
  smallest_side = tf.convert_to_tensor(smallest_side, dtype=tf.int32)
  height = tf.to_float(height)
  width = tf.to_float(width)
  smallest_side = tf.to_float(smallest_side)
  scale = tf.cond(tf.greater(height, width),
                  lambda: smallest_side / width,
                  lambda: smallest_side / height)
  new_height = tf.to_int32(height * scale)
  new_width = tf.to_int32(width * scale)

  float_caster = tf.cast(image_reader, tf.float32)
  dims_expander = tf.expand_dims(float_caster, 0)
  resized = tf.image.resize_bilinear(dims_expander, [new_height, new_width])
  
  resized = tf.squeeze(resized)
  resized.set_shape([None, None, 3])

  cropped = _central_crop(resized, output_height, output_width) 

  cropped.set_shape([output_height, output_width, 3])

  image = tf.to_float(cropped)

  normalized = _mean_image_subtraction(image, [_R_MEAN, _G_MEAN, _B_MEAN])

  normalized = tf.expand_dims(normalized, 0)

  sess = tf.Session()
  result = sess.run(normalized)

  return result


def load_labels(label_file):
  label = []
  proto_as_ascii_lines = tf.gfile.GFile(label_file).readlines()
  for l in proto_as_ascii_lines:
    label.append(l.rstrip())
  return label

if __name__ == "__main__":
  file_name = "data/grace_hopper.jpg"
  model_file = \
    "model/frozen_mobilenet.pb"
  label_file = "data/imagenet_slim_labels.txt"
  input_height = 224
  input_width = 224
  input_mean = 128 
  input_std = 128
  input_layer = "input"
  output_layer = "MobilenetV2/Predictions/Reshape_1"
  vggpreprocessing = 0

  parser = argparse.ArgumentParser()
  parser.add_argument("--image", help="image to be processed")
  parser.add_argument("--graph", help="graph/model to be executed")
  parser.add_argument("--labels", help="name of file containing labels")
  parser.add_argument("--input_height", type=int, help="input height")
  parser.add_argument("--input_width", type=int, help="input width")
  parser.add_argument("--input_mean", type=int, help="input mean")
  parser.add_argument("--input_std", type=int, help="input std")
  parser.add_argument("--input_layer", help="name of input layer")
  parser.add_argument("--output_layer", help="name of output layer")
  parser.add_argument("--vggpreprocessing", type=int, help="vggpreprocessing:0 inception style preprocessing(default); 1:vgg style preprocessing")
  
  args = parser.parse_args()

  if args.graph:
    model_file = args.graph
  if args.image:
    file_name = args.image
  if args.labels:
    label_file = args.labels
  if args.input_height:
    input_height = args.input_height
  if args.input_width:
    input_width = args.input_width
  if args.input_mean:
    input_mean = args.input_mean
  if args.input_std:
    input_std = args.input_std
  if args.input_layer:
    input_layer = args.input_layer
  if args.output_layer:
    output_layer = args.output_layer
  if args.vggpreprocessing:
    vggpreprocessing = args.vggpreprocessing  

  print(model_file)
  graph = load_graph(model_file)

  if vggpreprocessing == 0:
    t = read_tensor_from_image_file(file_name,
                                  input_height=input_height,
                                  input_width=input_width,
                                  input_mean=input_mean,
                                  input_std=input_std)
  elif vggpreprocessing == 1:
    t = read_tensor_from_image_file_vgg(file_name,
                                  output_height=input_height,
                                  output_width=input_width,
                                  input_mean=input_mean,
                                  input_std=input_std) 
  else:
    print("vggpreprocessing has to be 0 or 1");
    exit() 

  input_name = "import/" + input_layer
  output_name = "import/" + output_layer
  input_operation = graph.get_operation_by_name(input_name);
  output_operation = graph.get_operation_by_name(output_name);

  with tf.Session(graph=graph) as sess:
    results = sess.run(output_operation.outputs[0],
                      {input_operation.outputs[0]: t})
  results = np.squeeze(results)

  top_k = results.argsort()[-5:][::-1]
  labels = load_labels(label_file)


  for i in top_k:
    print(labels[i+vggpreprocessing], results[i])

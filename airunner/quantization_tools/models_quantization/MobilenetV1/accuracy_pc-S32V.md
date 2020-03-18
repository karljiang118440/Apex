




# EVB2 中精度测试结果：


Classifying: data/airunner/image_classification/dirver.jpg
Time taken to Inference times detect pictures:: 36 milliseconds
Top 5:
                  c1, 0.230696
                  c2, 0.085909
                  c3, 0.085474
                  c0, 0.085418
                   c, 0.085417
				   
				   
				   
				   
				   
# : compared the model accuracy in PC & s32v

python mnet_imagelabelling_test.py \
--image=$DATA_DIR/dirver.jpg  \
--graph=$MODEL_DIR/frozen_mb1_dms.pb  \
--labels=$MODEL_DIR/labels.txt  \
--input_height=224 --input_width=224

python mnet_imagelabelling_test.py \
--image=$DATA_DIR/dirver.jpg  \
--graph=$MODEL_DIR/frozen_mb1_dms_bn.pb  \
--labels=$MODEL_DIR/labels.txt  \
--input_height=224 --input_width=224


python mnet_imagelabelling_test.py \
--image=$DATA_DIR/dirver.jpg  \
--graph=$MODEL_DIR/frozen_mb1_dms_bn_qsym.pb  \
--labels=$MODEL_DIR/labels.txt  \
--input_height=224 --input_width=224


python mnet_imagelabelling_test.py \
--image=$DATA_DIR/dirver.jpg  \
--graph=$MODEL_DIR/frozen_mb1_dms_bn_qsym_final.pb  \
--labels=$MODEL_DIR/labels.txt  \
--input_height=224 --input_width=224


c0 0.9996581
c3 0.00014288502
c5 0.000114419585
c4 6.6341076e-05
c1 9.690593e-06

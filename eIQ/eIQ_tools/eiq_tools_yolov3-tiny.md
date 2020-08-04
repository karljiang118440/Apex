

# 4.8

Quantizing Tiny YOLOv3

1. Pretrained floating point model obtained from ONNX Zoo
• Tiny YOLOv3

2. Optional Verify the model
$cd $EIQ_AUTO_DATA/models/onnx/yolov3-tiny/workspace
$python tiny_yolo.py

3. Split the graph using eIQ Auto Graph Split

4. Quantize obtained subgraph
$cd $EIQ_AUTO_DATA/models/onnx/resnet18v1/workspace
$python quantize_model.py
The output: yolov3-tiny_annotate_minmax.onnx is the model to be used on EVB/APEX.

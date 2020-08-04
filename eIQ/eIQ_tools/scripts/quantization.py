#from quantize import Quantize
from eiq_auto import make_quantize
quant = make_quantize('frozen_inference_graph.pb')
quant.annotate_minmax('frozen_inference_graph.pb')
quant.export_model('.')



'''

## Quantization

```python
from eiq_auto import make_quantize
quant = make_quantize('/path/to/the/model_file')

# quantize the model running inference
quant.annotate_minmax(input_data)

# export the quantized model
quant.export_model(output_dir)
```

'''

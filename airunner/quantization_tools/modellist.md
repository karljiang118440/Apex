


frozen_mobilenet_q_sym_final_part.pb
frozen_mobilenet_float_outputlayers_graph.pb

frozen_mssd_afterbnfold_iden_part_quant_in.pb














frozen_mobilenet.pb  > frozen_mobilenet_bn.pb > frozen_mobilenet_bn_qsym.pb >  frozen_mobilenet_bn_qsym_final.pb
																			>  frozen_mobilenet_bn_qsym_final_part.pb
																			
																	
The final pb from mobilenet ,what the means of two model?
	
raw demos ,the models :
（1）.frozen_mobilenet_q_sym_final_part.pb 


mobilenet_loop("data/airunner/frozen_mobilenet_q_sym_final_part.pb",
                              "data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",
                              "data/airunner/image_classification/description.txt",
                              "data/airunner/image_classification/imagenet_slim_labels.txt");
							  
							  
							  
reload models:
							  
case_mobilenet("data/airunner/frozen_mobilenet_q_sym_final_part.pb",
               "data/airunner/frozen_mobilenet_float_outputlayers_graph.pb",
                              "data/airunner/test_classification.jpg",
                              "data/airunner/image_classification/imagenet_slim_labels.txt");
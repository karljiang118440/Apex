  
  
  #// Load graph from protobuf model
  
  std::vector<Tensor*> lOutputTensors;
  status = LoadGraphFromONNX(*net_BiSeNet, aBiSeNetGraph, lNetInput, lOutputTensors);
  if(Status_t::SUCCESS != status || lOutputTensors.empty())
  {
    std::cout << "Failed to load net" << std::endl;
    return {};
  }
  std::cout << "done loading graph" << std::endl;

  if(aFixed)
  {
    status =  net_BiSeNet->AllowNodeFusion(false);
    if(Status_t::SUCCESS != status)
    {
      std::cout << "failed to disable fusion" << std::endl;
      return {};
    }

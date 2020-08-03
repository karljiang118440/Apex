


# configure for tensor 
  if(aFixed)
  {
    lNetInput->Allocate(Allocation_t::OAL);
    lNetInput->SetQuantParams({QuantInfo(-4, 3)});
    lInputTensorChild1.Configure<>(DataType_t::SIGNED_8BIT, TensorShape<TensorFormat_t::NHWC>{1, inputH, inputW, 3});
    lInputTensorChild1.SetParent(lNetInput, {0, 3, 3, 0});
    lInputTensorChild1.SetQuantParams({QuantInfo(-4, 3)});
  }
  else
  {
    lNetInput->Allocate(Allocation_t::HEAP);
    lInputTensorChild1.Configure<>(DataType_t::FLOAT, TensorShape<TensorFormat_t::NHWC>{1, inputH, inputW, 3});
    lInputTensorChild1.SetParent(lNetInput, {0, 3, 3, 0});
  }

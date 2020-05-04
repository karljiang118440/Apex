/*im2col_cpu将c个通道的卷积层输入图像转化为c个通道的矩阵，矩阵的行值为卷积核高*卷积核宽，
也就是说，矩阵的单列表征了卷积核操作一次处理的小窗口图像信息；而矩阵的列值为卷积层
输出单通道图像高*卷积层输出单通道图像宽，表示一共要处理多少个小窗口。
im2col_cpu接收13个参数，分别为输入数据指针(data_im)，卷积操作处理的一个卷积组的通道
数(channels)，输入图像的高(height)与宽(width)，原始卷积核的高(kernel_h)与宽(kernel_w)，
输入图像高(pad_h)与宽(pad_w)方向的pad，卷积操作高(stride_h)与宽(stride_w)方向的步长，
卷积核高(stride_h)与宽(stride_h)方向的扩展，输出矩阵数据指针(data_col)*/
template <typename Dtype>
void im2col_cpu(const Dtype* data_im, const int channels,
    const int height, const int width, const int kernel_h, const int kernel_w,
    const int pad_h, const int pad_w,
    const int stride_h, const int stride_w,
    const int dilation_h, const int dilation_w,
    Dtype* data_col) {
  const int output_h = (height + 2 * pad_h -
    (dilation_h * (kernel_h - 1) + 1)) / stride_h + 1;//计算卷积层输出图像的高
  const int output_w = (width + 2 * pad_w -
    (dilation_w * (kernel_w - 1) + 1)) / stride_w + 1;//计算卷积层输出图像的宽
  const int channel_size = height * width;//计算卷积层输入单通道图像的数据容量
  /*第一个for循环表示输出的矩阵通道数和卷积层输入图像通道是一样的，每次处理一个输入通道的信息*/
  for (int channel = channels; channel--; data_im += channel_size) {
	/*第二个和第三个for循环表示了输出单通道矩阵的某一列，同时体现了输出单通道矩阵的行数*/
    for (int kernel_row = 0; kernel_row < kernel_h; kernel_row++) {
      for (int kernel_col = 0; kernel_col < kernel_w; kernel_col++) {
        int input_row = -pad_h + kernel_row * dilation_h;//在这里找到卷积核中的某一行在输入图像中的第一个操作区域的行索引
		/*第四个和第五个for循环表示了输出单通道矩阵的某一行，同时体现了输出单通道矩阵的列数*/
        for (int output_rows = output_h; output_rows; output_rows--) {
          if (!is_a_ge_zero_and_a_lt_b(input_row, height)) {//如果计算得到的输入图像的行值索引小于零或者大于输入图像的高(该行为pad)
            for (int output_cols = output_w; output_cols; output_cols--) {
              *(data_col++) = 0;//那么将该行在输出的矩阵上的位置置为0
            }
          } else {
            int input_col = -pad_w + kernel_col * dilation_w;//在这里找到卷积核中的某一列在输入图像中的第一个操作区域的列索引
            for (int output_col = output_w; output_col; output_col--) {
              if (is_a_ge_zero_and_a_lt_b(input_col, width)) {//如果计算得到的输入图像的列值索引大于等于于零或者小于输入图像的宽(该列不是pad)
                *(data_col++) = data_im[input_row * width + input_col];//将输入特征图上对应的区域放到输出矩阵上
              } else {//否则，计算得到的输入图像的列值索引小于零或者大于输入图像的宽(该列为pad)
                *(data_col++) = 0;//将该行该列在输出矩阵上的位置置为0
              }
              input_col += stride_w;//按照宽方向步长遍历卷积核上固定列在输入图像上滑动操作的区域
            }
          }
          input_row += stride_h;//按照高方向步长遍历卷积核上固定行在输入图像上滑动操作的区域
        }
      }
    }
  }
}


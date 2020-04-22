import numpy as np
input_data=[
              [[1,0,1,2,1],
               [0,2,1,0,1],
               [1,1,0,2,0],
               [2,2,1,1,0],
               [2,0,1,2,0]],
 
               [[2,0,2,1,1],
                [0,1,0,0,2],
                [1,0,0,2,1],
                [1,1,2,1,0],
                [1,0,1,1,1]] 
            ]
weights_data=[ 
               [[ 1, 0, 1],
                [-1, 1, 0],
                [ 0,-1, 0]],
               [[-1, 0, 1],
                [ 0, 0, 1],
                [ 1, 1, 1]] 
 
           ]
 
#fm:[h,w]
#kernel:[k,k]
#return rs:[h,w] 
def compute_conv(fm,kernel):
    [h,w]=fm.shape
    [k,_]=kernel.shape 
    r=int(k/2)
    #定义边界填充0后的map
    padding_fm=np.zeros([h+2,w+2],np.float32)
    #保存计算结果
    rs=np.zeros([h,w],np.float32)
    #将输入在指定该区域赋值，即除了4个边界后，剩下的区域
    padding_fm[1:h+1,1:w+1]=fm 
    #对每个点为中心的区域遍历
    for i in range(1,h+1):
        for j in range(1,w+1): 
            #取出当前点为中心的k*k区域
            roi=padding_fm[i-r:i+r+1,j-r:j+r+1]
            #计算当前点的卷积,对k*k个点点乘后求和
            rs[i-1][j-1]=np.sum(roi*kernel)
 
    return rs
def my_conv2d(input,weights):
    [c,h,w]=input.shape
    [_,k,_]=weights.shape
    outputs=np.zeros([h,w],np.float32)
 
    #对每个feature map遍历，从而对每个feature map进行卷积
    for i in range(c):
        #feature map==>[h,w]
        f_map=input[i]
        #kernel ==>[k,k]
        w=weights[i]
        rs =compute_conv(f_map,w)
        outputs=outputs+rs   
    return outputs
def main():  
    #shape=[c,h,w]
    input = np.asarray(input_data,np.float32)
    #shape=[in_c,k,k]
    weights =  np.asarray(weights_data,np.float32) 
    rs=my_conv2d(input,weights) 
    print(rs) 
 
 
if __name__=='__main__':
    main() 
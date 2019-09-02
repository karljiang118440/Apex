

#ifndef AIRUNNERCLASSIFY_H_
#define AIRUNNERCLASSIFY_H_

#include <string>



class AIRunner_Classify
{
  public:
   AIRunner_Classify(void);
   ~AIRunner_Classify(void);

  private:

    Tensor* lApexNetInput;
    std::vector<Tensor*>fixedOutput;
    Tensor* floatInterm;

    std::unique_ptr<Graph> net_mobile;
    std::unique_ptr<Graph> netFixedToFloat;

    std::vector<Tensor*> Output;
    std::vector<std::string> classLabels;

  public:

  int case_mobilenet(const std::string& aMnetGraph,const std::string& aImageFile,const std::string& aSlimLabelsFile);
  std::vector<std::pair<int,float>> case_mobilenet_target()
}
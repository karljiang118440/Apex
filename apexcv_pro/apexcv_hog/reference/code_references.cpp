


//Part1:gdc_pd_hog_task.cpp


    /***************************************************************************/
    //  Initialization Routine for HOG Score Buffer
    /***************************************************************************/

    intResult = mHogRef.SetSVM(mSvmDouble);



  //
  // APEX Hog
  //

  for(uint32_t i = 0; i < mNumberOfSceneResizes; ++i)
  {
    SHOW_ERROR(rData.mSceneResizes_umat[i].cols == mSceneResizes[i].mWidth &&
               rData.mSceneResizes_umat[i].rows == mSceneResizes[i].mHeight &&
               rData.mSceneResizes_umat[i].step[0] == mSceneResizes[i].mSpan);

    if(mWeUseApexHog == true)
    {
      AcfProfilingInfo lAcfProfilingInfo;
      uint32_t         lStart;
      uint32_t         lStop;
      ticks_per[i][0] = FSL_Ticks();

      if(!mIsApexHogInitialized)
      {
        intResult = mHog.Initialize(rData.mSceneResizes_umat[i], mHogScores_umat, apexcv::Hog::HogType::DETECT);
        SHOW_ERROR(intResult == 0);

        intResult = mHog.SetSVM(mSvmDouble);
        SHOW_ERROR(intResult == 0);

        intResult = mHog.SelectApexCore(gcApexCvHogApexId);
        SHOW_ERROR(intResult == 0);

        mIsApexHogInitialized = true;
      }
      else
      {
        mHog.ReconnectIO(rData.mSceneResizes_umat[i], mHogScores_umat);
      }


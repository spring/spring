#include "openh264.h"	


StreamingController::StreamingController(inet 
ipAdress, unsigned int port){
	//open bit stream
	
	outStream  = new std::stream()<	


}

StreamingController::initStream() {
	int rv = WelsCreateSVCEncoder (&encoder_);
	ASSERT_EQ (0, rv);
	ASSERT_TRUE (encoder_ != NULL);

	SEncParamBase param;
	memset (&param, 0, sizeof (SEncParamBase));
	param.iUsageType = usageType;
	param.fMaxFrameRate = frameRate;
	param.iPicWidth = width;
	param.iPicHeight = height;
	param.iTargetBitrate = 5000000;
	encoder_->Initialize (&param);

	int videoFormat = videoFormatI420;
	encoder_->SetOption (ENCODER_OPTION_DATAFORMAT, 
	&videoFormat);

	//fill in instant param content
	encoder_->SetOption 
	(ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, &param);


}

StreamingController::EncodePicture(){
	int frameSize = width * height * 3 / 2;
	BufferedData buf;
	buf.SetLength (frameSize);
	ASSERT_TRUE (buf.Length() == (size_t)frameSize);
	SFrameBSInfo info;
	memset (&info, 0, sizeof (SFrameBSInfo));
	SSourcePicture pic;
	memset (&pic, 0, sizeof (SsourcePicture));
	pic.iPicWidth = width;
	pic.iPicHeight = height;
	pic.iColorFormat = videoFormatI420;
	pic.iStride[0] = pic.iPicWidth;
	pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
	pic.pData[0] = buf.data();
	pic.pData[1] = pic.pData[0] + width * height;
	pic.pData[2] = pic.pData[1] + (width * height >> 2);
	for(int num = 0;num<total_num;num++) {
	   //prepare input data
	   rv = encoder_->EncodeFrame (&pic, &info);
	   ASSERT_TRUE (rv == cmResultSuccess);
	   if (info.eFrameType != videoFrameTypeSkip && cbk != 
	NULL) {
		//TODOo
	outStream =  current ImageBuffer;
	    //output bitstream
	   }


}


StreamingController::Dismantle(){
	
	if (encoder_) { 
		encoder_->Uninitialize(); 
		WelsDestroySVCEncoder (encoder_);
	 }

}
